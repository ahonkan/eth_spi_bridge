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
*       chkdsk.c
*
* COMPONENT
*
*       FAT Check Disk Utility
*
* DESCRIPTION
*
*       This file contains the major functions for Nucleus FILE's FAT 
*       Check Disk Utility.
*       This utility can be used to check for the following 
*       issues:
*               Lost Cluster Chains
*               Cross-linked Chains
*               Invalid file lengths
*               Damaged Directory Record entries
*               Mismatch FAT Tables
*               
*
* DATA STRUCTURES
*
*       None
*
* FUNCTIONS
*
*       chk_lost_cl_chains_find_start           Finds the start of a 
*                                               lost cluster chains once
*                                               it have been identified.
*       chk_lost_cl_chain_resolve               Resolves the lost cluster
*                                               chain issues.
*       chk_lost_cl_chain_test                  Checks the drive for lost 
*                                               cluster chains.
*       chk_crosslink_test                      Checks the drive for 
*                                               cross-linked chains.
*       chk_crosslink_check_disk                Used by the 
*                                               chk_traverse_all_dir_paths
*                                               to determine if a cluster
*                                               is cross linked.
*       chk_crosslink_resolve                   Resolves the cross-linked
*                                               cluster chains.
*       chk_create_error_record                 Creates a directory record
*                                               that is to be linked with
*                                               an error cluster chain.
*       chk_fat_table_compare_test              Used to compare the FAT
*                                               tables.
*       chk_lfn_traverse_records                Traverse a LFN one directory
*                                               record at a time.
*       chk_lfn_del_records                     Deletes a LFN directory
*                                               record.
*       chk_lfn_check                           Checks a LFN directory
*                                               records for errors.
*       chk_check_sfn_chars                     Checks the SFN for invalid
*                                               characters.
*       chk_check_dir                           Driver function for checking
*                                               the directory records.
*       chk_check_cl_value                      Verifies the cluster value
*                                               passed in is valid.
*       chk_dir_rec_entry_test                  Checks the directory records
*                                               for errors. 
*       chk_dir_rec_entry_test_op               Used by chk_traverse_all_dir_paths
*                                               to check for damaged directory 
*                                               entries.                                           
*       chk_file_size_test                      Driver function for checking
*                                               files sizes.
*       chk_file_size_test_op                   Used by chk_traverse_all_dir_paths
*                                               to check for incorrect file sizes
*       chk_calc_file_size                      Calculates the size of a file.
*       chk_calc_size_on_disk                   Calculates the size of the file
*                                               on disk.                            
*       chk_find_end_of_data_sector             Finds where data stops
*                                               in a sector.  
*       chk_change_iv_cl_in_fat_to_eoc          Check the FAT table for
*                                               clusters value that is
*                                               reserved or out of range.
*       chk_validate_all_cl_chains_on_disk      Checks every directory
*                                               records' cluster chain
*                                               on disk for BAD or FREE
*                                               clusters in chain.
*       chk_validate_cl_chains                  Checks the directory
*                                               records' cluster chain
*                                               for BAD or FREE values.
*       chk_handle_bad_cl_in_cl_chain           Handles case when BAD
*                                               cluster is found in cluster 
*                                               chain.
*       chk_handle_free_cl_in_cl_chain          Handles case when FREE
*                                               cluster is found in cluster 
*                                               chain.
*
************************************************************************/
#include    "storage/chkdsk_extr.h"
#include    "storage/chkdsk_util.h"
#include    "storage/chkdsk_log.h"

#if(CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1)

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

/************************************************************************/
/*  Globals                                                             */
/************************************************************************/

UINT8 *g_chkdsk_fat_bitmap;    /* Only used with cross-link and lost cluster chain check. */
UINT8 g_chkdsk_mode;           /* Used by the Check Disk Utility so it knows whether to attempt to fix 
                           problems found on the disk or not. */

extern UINT32 FILE_Unused_Param; /* Used to prevent compiler warnings */
extern LOG_RECORD g_chkdsk_log_rec[MAX_NUM_LOG_RECORD_ENTRIES]; 
/************************************************************************
* FUNCTION
*
*   chk_lost_cl_chains_find_start
*
* DESCRIPTION
*
*   This function traverses the g_chkdsk_fat_bitmap checking to see if any
*   clusters value in the bitmap are CL_ISSUE or NOT_START_OF_LOST_CL_CHAIN. 
*   If the cluster bitmap value is one of these two then that clusters 
*   value is marked in the bitmap as NOT_START_OF_LOST_CL_CHAIN. By doing
*   this we know that all the clusters whose bitmap value are CL_ISSUE are
*   the start cluster of a lost cluster chain. This function assumes 
*   that the g_chkdsk_fat_bitmap has already flagged all the clusters that 
*   are on the disk, and the clusters that are part of a lost cluster chain. 
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*    NU_SUCCESS                 Found the start of all the
*                               lost cluster.                 
*
*************************************************************************/
STATUS chk_lost_cl_chains_find_start(UINT16 dh)
{
    STATUS ret_val;
    UINT32 i;
    UINT32 value;
    UINT8 bitmap_cl_value;
    FAT_CB *pfs_cb = NU_NULL;
    DDRIVE *pddrive;
    UINT32 eoc_marker;

    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) && pfs_cb != NU_NULL)
    {
        pddrive = pfs_cb->ddrive;
        for(i = 2; i < pddrive->maxfindex; ++i)
        {          
            bitmap_cl_value = chk_bitmap_get_value(i);
            /* Found a cluster in a lost cluster chain, now see if someone points to it. */
            if((bitmap_cl_value == CL_ISSUE) || (bitmap_cl_value == NOT_START_OF_LOST_CL_CHAIN))
            {
                PC_FAT_ENTER(dh)
                ret_val = pc_faxx(pddrive, i, &value);
                PC_FAT_EXIT(dh)            

                if(ret_val == NU_SUCCESS)
                {
                    /* Make sure the value is valid. */
                    ret_val = chk_is_cl_valid(pddrive,value);

                    /* If the value points to someone, then mark it as VALID_ON_DISK. 
                       By doing this we will end up with all the lost cluster chains
                       start cluster as being marked as CL_ISSUE in our bitmap. */
                    if((ret_val == NU_SUCCESS) && value)
                    {         
                        if(ret_val == NU_SUCCESS)
                        {
                            eoc_marker = chk_get_eoc_value_for_drive(pddrive);

                            if(value < eoc_marker)
                                ret_val = chk_bitmap_set_value(value, NOT_START_OF_LOST_CL_CHAIN);                    
                        }
                    }
                }
            }
        }
    }

    return ret_val;

}

/************************************************************************
* FUNCTION
*
*   chk_lost_cl_chain_resolve
*
* DESCRIPTION
*
*   This function traverses the g_chkdsk_fat_bitmap checking to see if there
*   are any lost cluster chains. If there are then, a directory is created 
*   to store all the lost cluster chain files/directories. The directory 
*   name is based of the constant CHK_FOLDER_FOR_LOST_CL_CHAIN_ENTRIES. Next, 
*   if the lost cluster chain is a file then, a file is created in the 
*   CHK_FOLDER_FOR_LOST_CL_CHAIN_ENTRIES directory. If the lost cluster chain is a
*   directory, then its self and all of its subdirectories will have a entry 
*   in the error directory.
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  All the lost cluster chains where
*                               resolved.                  
*
*************************************************************************/
STATUS chk_lost_cl_chain_resolve(UINT16 dh)
{
    STATUS ret_val;
    UINT32 i;
    UINT32 value;
    UINT8 buff[CHKDSK_SECTOR_SIZE];
    UINT32 sec;    
    UINT16 err_fl_num = 0;
    UINT16 err_dir_num = 0;
    UINT32 fl_calc_sz = 0;    
    UINT8 bitmap_cl_value;    
    FAT_CB *pfs_cb = NU_NULL;
    DDRIVE *pddrive;
#if (CHKDSK_DEBUG == NU_TRUE)
    CHAR cl_num_str[13];
#endif
    /* Add one for /0 and one for '.' */
    CHAR err_dir_name[MAX_SFN+MAX_EXT + 2] = {CHK_FOLDER_FOR_LOST_CL_CHAIN_ENTRIES};

    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
    {
        pddrive = pfs_cb->ddrive;    
        
        /* Loop though the bitmap(which is a representation of a FAT Table),
           checking for the start clusters of lost cluster chains. */
        for(i = 2; ((i < pddrive->maxfindex) && (ret_val == NU_SUCCESS));++i )
        {
            /* Get the bitmap's value for this cluster. */
            bitmap_cl_value = chk_bitmap_get_value(i);
            
            /* Found start cluster of lost cluster chain. */
            if((bitmap_cl_value != NOT_START_OF_LOST_CL_CHAIN) &&
               (bitmap_cl_value != VALID_ON_DISK) && (bitmap_cl_value != BITMAP_FREE_CL))
            {   
                PC_FAT_ENTER(dh)
                ret_val = pc_faxx(pddrive, i, &value);
                PC_FAT_EXIT(dh)                            
                
                if(ret_val == NU_SUCCESS)    
                {
                    
#if CHKDSK_DEBUG
                    PRINTF("Lost Cluster Chain: ",&port);
                    chk_int_to_str(&cl_num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,i);
                    PRINTF(&cl_num_str[0],&port);
                    PRINTF("\n\r",&port);
#endif                        
                    /* Log our errors. */				 
                    if(chk_log_is_error_num_zero(CHKDSK_LOGID_LOST_CL_CHAIN) == NU_TRUE)
                    {   
                        /* Start the logger. */
                        ret_val = chk_start_test_case((UINT8)CHK_CHECK_LOST_CL_CHAIN);

                        /* If we fail to write start of lost cluster chain test case
                           in our logger then break loop and return error. */
                        if(ret_val != NU_SUCCESS)
                        {
                            break;
                        }
                    }
                                         
                    chk_log_add_error(CHKDSK_LOGID_LOST_CL_CHAIN);
                    

                    /* Are we fixing errors? If not don't create the directory and don't
                       create an error file/directory for it. */
                    if(g_chkdsk_mode != CHK_FIX_ERRORS)
                    {
                        continue;                        
                    }
                    
                
                    /* Create an error folder for the lost cluster chain entries
                       then set the drive to it, so the error entries are 
                       created in it. */
                    if((err_fl_num == 0) && (err_dir_num == 0))
                    {
                        /* Release our drive lock so we can do high level file operations. */
                        PC_DRIVE_EXIT(dh)
                        /* Make sure we are in the root directory. */
                        ret_val = NU_Set_Current_Dir("\\");
                        if(ret_val == NU_SUCCESS)
                        {
                            ret_val = chk_make_err_dir(&err_dir_name[0]);
                            if(ret_val == NU_SUCCESS)
                            {
                                ret_val = NU_Set_Attributes(&err_dir_name[0], ADIRENT | ASYSTEM | AHIDDEN);
                                if(ret_val == NU_SUCCESS)
                                {
                                    ret_val = NU_Set_Current_Dir(&err_dir_name[0]);
                                    /* If an error occurs when setting the directory stop looking for lost
                                       cluster chains and report the error. */
                                    if(ret_val != NU_SUCCESS)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        /* Reacquire our drive lock. */
                        PC_DRIVE_ENTER(dh,YES)                    
                    }
                

                    /* Read in a sector of the cluster to determine if it is a file
                       or a directory. */    
                    sec = pc_cl2sector(pddrive, i);                                             

                    PC_DRIVE_IO_ENTER(dh)
                    ret_val = fs_dev_io_proc(dh, sec, &buff[0], 1, YES);
                    PC_DRIVE_IO_EXIT(dh)
                    
                    if(ret_val == NU_SUCCESS)
                    {                        
#if (CHKDSK_LCC_ERROR_FILES_ONLY == NU_FALSE)                            
                        /* If the first directory record is a dot and the second directory
                           record is a dot dot, and dot's directory record cluster value matches
                           the cluster value of the first cluster in the lost cluster chain, then
                           we assume this is a directory. */
                        if((buff[0] == '.') && (buff[32] == '.') && (buff[33] =='.') && 
                           (chk_get_dir_rec_cl(pddrive, (DOSINODE*)&buff[0]) == i))
                        {
                            ret_val = chk_create_error_record(dh, pddrive, &err_dir_name[0], 
                                                              CHK_LOST_CL_CHAIN_DIR_NAME, err_dir_num++, i, 0, NU_FALSE);
                    
                        }
                        /* If it isn't a directory, only one other option, it must be a file. */
                        else
#endif
                        {
#if (CHKDSK_LCC_ERROR_FILES_ONLY == NU_TRUE)  
                            ret_val = chk_calc_file_size(&fl_calc_sz, dh, i);
                            if(fl_calc_sz < pddrive->bytespcluster)
                            {
                                fl_calc_sz = pddrive->bytespcluster;
                            }
#endif
                
                            if(ret_val == NU_SUCCESS)
                            {
                                ret_val = chk_create_error_record(dh, pddrive, &err_dir_name[0], 
                                                                  CHK_LOST_CL_CHAIN_FL_NAME, err_fl_num++, i, fl_calc_sz, NU_TRUE);                       
                        
                                /* If error found break loop and report error. */
                                if(ret_val != NU_SUCCESS)
                                {
                                    break;
                                }
                            }
                            /* If error found break loop and report error. */
                            else
                            {
                                break;
                            }
            
                        }
                    }
                    
                }
            }
        }/* End of for loop through clusters. */

        if(ret_val == NU_SUCCESS)
        {
            /* Release our drive lock so we can do high level file operations. */
            PC_DRIVE_EXIT(dh)
            /* Make sure we set the current directory back to root. */
            ret_val = NU_Set_Current_Dir("\\");
            /* Reacquire our drive lock. */
            PC_DRIVE_ENTER(dh,YES)       
            
        }
    }

    return ret_val;
    
}

/************************************************************************
* FUNCTION
*
*   chk_lost_cl_chain_test
*
* DESCRIPTION
*
*   This is the driver function for the lost cluster chain test. First, 
*   it will traverse the directory hierarchy on the disk, marking 
*   each cluster encountered in our bitmap as VALID_ON_DISK. Then, if it
*   is a FAT32 drive it marks all the root directory clusters as 
*   VALID_ON_DISK. Next, it will traverse our bitmap marking each cluster 
*   in our bitmap as CL_ISSUE that is marked as free in our bitmap, but 
*   actually contains a value in the FAT table. So now in our bitmap  
*   all the clusters on the disk are marked as VALID ON DISK and all the 
*   lost cluster chains as CL_ISSUE. Traverse the bitmap again, marking 
*   each entries value as NOT_START_OF_LOST_CL_CHAIN NOT.
*   (NOTE: Very important it isn't marking the cluster in the bitmap as 
*   NOT_START_OF_LOST_CL_CHAIN but the value that the cluster contains as 
*   NOT_START_OF_LOST_CL_CHAIN). By doing this any cluster whose value in 
*   the bitmap is CL_ISSUE is the start cluster of a lost cluster chain. 
*   The chains are created as follows:
*
*       First a directory is created to store all the lost cluster chain 
*       files/directories. The directory is name whose value
*       is stored in this constant CHK_FOLDER_FOR_LOST_CL_CHAIN_ENTRIES.
*       
*       Then, if the lost cluster chain is a file a file is created in 
*       the error directory. If the lost cluster chain is a directory, 
*       then its self and all of its subdirectories will have a 
*       entries in the CHK_LOST_CL_CHAIN_DIR_NAME directory.
*
*       Logger: Test Case is logged in chk_lost_cl_chain_resolve if 
*               a lost cluster chain is found. This test case
*               ends its log here in chk_lost_cl_chain_test if
*               any lost cluster chains where found.   
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Checks the disk for lost cluster
*                               chains.              
*
*************************************************************************/
STATUS chk_lost_cl_chain_test(UINT16 dh)
{
    STATUS ret_val;
    FAT_CB *pfs_cb = NU_NULL;     

    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
    {        
        /* Make sure our global list of log records gets cleared,
           Need to clear it before each test to make room for
           current test possible errors. */
        CHK_CLEAR_LOG_REC();

         /* Allocate a bitmap with 2 bits being equal to one FAT table entry. Add 1 encase there is a remainder. */
        g_chkdsk_fat_bitmap = NUF_Alloc(((pfs_cb->ddrive->maxfindex/(MAX_BITS_PER_BITMAP_BLOCK/MAX_BITS_PER_BITMAP_ENTRY))+1) * sizeof(UINT8));

        /* Make sure it is zeroed out. */
        NUF_Memfill(g_chkdsk_fat_bitmap,((pfs_cb->ddrive->maxfindex/(MAX_BITS_PER_BITMAP_BLOCK/MAX_BITS_PER_BITMAP_ENTRY))+1) * sizeof(UINT8), BITMAP_FREE_CL);

        /* Traverse the whole directory hierarchy flagging each cluster in our bitmap 
           as VALID_ON_DISK, or LOST_CLUSTER if the cluster is already marked as VALID_ON_DISK. */
        PC_DRIVE_ENTER(dh,NO)

        ret_val = chk_traverse_all_dir_paths(dh, chk_bitmap_mark_cl_on_disk, NU_TRUE, NU_TRUE);

        PC_DRIVE_EXIT(dh)

        if(ret_val == NU_SUCCESS)
        {    
            /* If we are using FAT32 we need to exclude the root directory, so mark it as
                   VALID_ON_DISK. */
            if(CHK_IS_DRIVE_FAT32(pfs_cb->ddrive))
            {
                UINT32 cl = pfs_cb->ddrive->rootdirstartcl;
                PC_FAT_ENTER(dh)
                do 
                {

                    ret_val = chk_bitmap_set_value(cl, VALID_ON_DISK);
                	if(ret_val == NU_SUCCESS)
                    {
                        ret_val = pc_clnext(&cl, pfs_cb->ddrive, cl);                        
                    }
                    
                } while(cl && ret_val == NU_SUCCESS);
                PC_FAT_EXIT(dh)
            }


            /* Check FAT for lost cluster chains. Any cluster in our bitmap
               marked as BITMAP_FREE_CL, but contains a value is marked in our bitmap
               as CL_ISSUE. */
            ret_val = chk_bitmap_replace_if(dh, BITMAP_FREE_CL, CL_ISSUE);
            if(ret_val == NU_SUCCESS)
            {
                /* Find first cluster of each lost cluster chain. */                    
                ret_val = chk_lost_cl_chains_find_start(dh);            
                if(ret_val == NU_SUCCESS)
                {
                    PC_DRIVE_ENTER(dh,YES)
                    /* Create error files/directories for the lost cluster chains. */
                    ret_val = chk_lost_cl_chain_resolve(dh);
                    PC_DRIVE_EXIT(dh)
                }
            }        
        }
        
        /* Attempt to free the resources that where allocated. However, if there is already an
           error don't check for an error because we want to try and return the earliest 
           error possible. */
        if(ret_val != NU_SUCCESS)
        {
            NU_Deallocate_Memory((VOID*)g_chkdsk_fat_bitmap);
        }
        else
        {
            ret_val = NU_Deallocate_Memory((VOID*)g_chkdsk_fat_bitmap);
            if(ret_val == NU_SUCCESS)
            {
                /* Check to see if any lost cluster chains where found. */
                if(chk_log_is_error_num_zero(CHKDSK_LOGID_LOST_CL_CHAIN)  == NU_FALSE)
                {
                    ret_val = chk_log_write_errors_to_file(CHKDSK_LOGID_LOST_CL_CHAIN);
                    if(ret_val == NU_SUCCESS)
                    {
                        /* Write our end tag to our logger for lost cluster chain test case. */
                        ret_val = chk_end_test_case();        
                    }
                }            
            }        
        }
    }    

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_crosslink_test
*
* DESCRIPTION
*
*   This is the driver function for checking the disk for cross-link
*   cluster chains. First, it traverses the directory hierarchy on
*   the disk, marking each cluster encountered in our bitmap as 
*   VALID_ON_DISK or CROSS_LINKED. A cluster entry is marked as CROSS_LINKED 
*   if it has been encountered before. After, this we traverse the directory 
*   hierarchy again, checking our bitmap for clusters marked as CROSS_LINKED. 
*   When a cross linked chain is encountered the following will occur:
*	
*       A directory or file will be created with the start cluster of the 
*       cross link chain and be put in a system/hidden folder named 
*       the value of the constant CHK_FOLDER_FOR_CROSS_LINK_ENTRIES.
*        
*       If directory records start cluster is same as the start cluster for 
*       cross-link chain, then that directory record is deleted.
*        
*       If the cross link chain is in a directory record's cluster chain, 
*       then in the directory record, mark the cluster before the start
*       cluster of the cross link chain as EOC.       
*       
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Test the disk for cross-link cluster
*                               chains.                  
*
*************************************************************************/
STATUS chk_crosslink_test(UINT16 dh)
{
    STATUS ret_val;
    FAT_CB *pfs_cb = NU_NULL;

    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
    {        
        /* Make sure our global list of log records gets cleared,
           Need to clear it before each test to make room for the
           current test possible errors. */
        CHK_CLEAR_LOG_REC();
     
        /* Allocate a bitmap with 2 bits being equal to one FAT table entry. Add 1 encase there is a remainder. */
        g_chkdsk_fat_bitmap = NUF_Alloc(((pfs_cb->ddrive->maxfindex/(MAX_BITS_PER_BITMAP_BLOCK/MAX_BITS_PER_BITMAP_ENTRY))+1) * sizeof(UINT8));

        /* Make sure it is zeroed out. */
        NUF_Memfill(g_chkdsk_fat_bitmap, ((pfs_cb->ddrive->maxfindex/(MAX_BITS_PER_BITMAP_BLOCK/MAX_BITS_PER_BITMAP_ENTRY))+1) * sizeof(UINT8), BITMAP_FREE_CL);

        PC_DRIVE_ENTER(dh,NO)
        /* Traverse the whole directory hierarchy flagging each cluster in our bitmap 
           as VALID_ON_DISK, or mark it as CROSS_LINKED if the cluster is already marked 
           as VALID_ON_DISK. */
        ret_val = chk_traverse_all_dir_paths(dh, chk_crosslink_check_disk, NU_TRUE, NU_FALSE);
        PC_DRIVE_EXIT(dh)

        if(ret_val == NU_SUCCESS)
        {   
            PC_DRIVE_ENTER(dh,YES)
            /* Now fix the cross linked files/directories. We want to gather LFN information
               on this pass in case we have to delete a directory record. */  
            ret_val = chk_traverse_all_dir_paths(dh, chk_crosslink_resolve, NU_FALSE, NU_FALSE);
            PC_DRIVE_EXIT(dh)
        }
    
        /* Attempt to free the resources that where allocated. However, if there is already an
           error don't check for error because we want to try and return the earliest 
           error possible. */
        if(ret_val != NU_SUCCESS)
        {
            (VOID)NU_Deallocate_Memory((VOID*)g_chkdsk_fat_bitmap);
        }
        else
        {
            ret_val = NU_Deallocate_Memory((VOID*)g_chkdsk_fat_bitmap);
            if(ret_val == NU_SUCCESS)
            {
                /* Check to see if any lost cluster chains where found. */
                if(chk_log_is_error_num_zero(CHKDSK_LOGID_CROSS_LINKED) == NU_FALSE)
                {
                    /* Log the total number of Cross-link Chains found. */
                    ret_val = chk_log_write_errors_to_file(CHKDSK_LOGID_CROSS_LINKED);
                    if(ret_val == NU_SUCCESS)
                    {
                        /* Write our our end tag for this test case. */
                        ret_val = chk_end_test_case();
                    }
                }
            }        
        }        
    }

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_crosslink_check_disk
*
* DESCRIPTION
*
*   This function is used by chk_traverse_all_dir_paths. This function
*   will be called for every directory record encountered. Each directory
*   record will be checked to see if it is or part of a cross-link chain.          
*
* INPUTS
*  
*   *pdir_rec_obj(in)            Directory Record Object.
*                               
*
* OUTPUTS
*
*  NU_SUCCESS                   All the cross-linked chains where
*                               marked as CROSS_LINKED.    
*  NUF_BADPARM                  pdir_rec_obj points to NU_NULL.               
*
*************************************************************************/
STATUS chk_crosslink_check_disk(DIR_REC_OBJ *pdir_rec_obj)
{
    STATUS ret_val = NU_SUCCESS;
    UINT32 cl;
    UINT32 nxt_cl;
    UINT8 bitmap_cl_value;
    FAT_CB *pfs_cb = NU_NULL; 

    if(pdir_rec_obj != NU_NULL)
    {
        /* Make sure it is a file or directory record. */
        if(pdir_rec_obj->pdos_rec->fattribute != CHK_LFN_ATTR)
        {    
            ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
            if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
            {
                /* Get the current directory records cluster value. */
                cl = chk_get_dir_rec_cl(pfs_cb->ddrive, pdir_rec_obj->pdos_rec);                       

                PC_FAT_ENTER(pdir_rec_obj->dh)
                /* Traverse this directory records cluster chain, marking each
                   cluster in our bitmap. */
                do 
                {                
                    ret_val = pc_clnext(&nxt_cl, pfs_cb->ddrive, cl);                
                    
                    if(ret_val == NU_SUCCESS)
                    {
                        ret_val = chk_is_cl_value_valid(pfs_cb->ddrive, cl);
                        if(ret_val == NUF_CHK_CL_INVALID)
                        {
                            break;
                        }
                        else
                        {
                            if(cl != 0x00)
                            {                       
                                bitmap_cl_value = chk_bitmap_get_value(cl);
                                if((bitmap_cl_value != VALID_ON_DISK) && (bitmap_cl_value != CROSS_LINKED))
                                {
                                    ret_val = chk_bitmap_set_value(cl, VALID_ON_DISK);
                                }
                                else
                                {
                                    bitmap_cl_value = chk_bitmap_get_value(pdir_rec_obj->pdos_rec_loc->cl_num);
                                    if(chk_bitmap_get_value(cl) != CROSS_LINKED && (bitmap_cl_value != CROSS_LINKED))
                                    {
                                        ret_val = chk_bitmap_set_value(cl, CROSS_LINKED);
                                    }
                                }
                
                            }
                        }
                    }
                    /* Set cl to the next clusters. */
                    cl = nxt_cl;                
            
                } while(cl && ret_val == NU_SUCCESS);            
                PC_FAT_EXIT(pdir_rec_obj->dh)        
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
*   chk_crosslink_resolve
*
* DESCRIPTION
*
*   This function will resolve the cross-link cluster chains
*   found on the drive. Cross-linked chains are resolved in the following
*   manner:
*    
*       If a directory record's start cluster is same as the start cluster 
*       for cross link chain, then that directory record is deleted.
*       If the cross link chain is in a directory records cluster chain, 
*       then in the directory record mark the cluster before the start 
*       cluster of the cross-link chain as EOC.
*
*       NOTE: Because we remove directory record's whose start cluster 
*             is the same as the cross-link chain's start cluster,
*             there is the possibility that the number of cross-link
*             chains found in fix mode and report only mode may differ.
*             Example: Folder1 cluster chain is 3
*                      Folder2 cluster chain is 6->3
*                      Folder3 cluster chain is 3
*                      Cluster 3 contains 5 files.
*                      This would result in report only mode reporting
*                      more cross-link chains, than fix mode. Because
*                      in fix mode Folder1 would have been deleted 
*                      before its contents was scanned which means
*                      its contents wouldn't be marked as cross-link
*                      even though it is. However, in fix mode the 
*                      contents is preserved because an error folder
*                      is created with it.
*          
*
* INPUTS
*  
*   pdir_rec_obj(in)                Pointer to a Directory Record Object.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Cross-linked chains where resolved.
*   NUF_BADPARM                 pdir_rec_obj points to NU_NULL.               
*
*************************************************************************/
STATUS chk_crosslink_resolve(DIR_REC_OBJ *pdir_rec_obj)
{
    STATUS ret_val;         
    UINT32 cl;    
    UINT32 prev_cl = 0;    
    UINT8 buff[CHKDSK_SECTOR_SIZE];
    static UINT16 error_fl_num = 0;
    static UINT16 error_dir_num = 0;    
    UINT32 num_cl_trav = 0;
    UINT32 fl_size;
    DOSINODE *pnew_dir_rec;
    FAT_CB *pfs_cb = NU_NULL;  
    DROBJ *pmom = NU_NULL;
    
#if (CHKDSK_DEBUG == NU_TRUE)
    CHAR cl_num_str[13];
#endif
    /* Add one for /0 and one for '.' */
    static CHAR err_dir_name[MAX_SFN + MAX_EXT + 2] = {CHK_FOLDER_FOR_CROSS_LINK_ENTRIES};
    
    if(pdir_rec_obj != NU_NULL)
    {
        ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
        if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
        {
            /* Get the current directory records cluster value. */
            cl = chk_get_dir_rec_cl(pfs_cb->ddrive, pdir_rec_obj->pdos_rec);
            /* Okay, use prev_cl to remember where the file stops, and
               cl to tell where the cross-linked cluster chain
               begins. */       
            do 
            {
                ret_val = chk_is_cl_value_valid(pfs_cb->ddrive, cl);
                if(ret_val == NUF_CHK_CL_INVALID)
                {
                    break;
                }
              
                if(chk_bitmap_get_value(cl) == CROSS_LINKED)
                {
#if CHKDSK_DEBUG
                    PRINTF("Cross-linked cluster: ",&port);
                    chk_int_to_str(&cl_num_str[0],CHK_MAX_DIGIT_IDX_IN_ERR_NAME,cl);
                    PRINTF(&cl_num_str[0],&port);
                    PRINTF("\n\r",&port);
#endif
                    /* Log our errors. */
                    if(chk_log_is_error_num_zero(CHKDSK_LOGID_CROSS_LINKED) == NU_TRUE)
                    {
                        /* Start the logger. */
                        ret_val = chk_start_test_case(CHK_CHECK_CROSS_LINKED_CHAIN);
                    
                        /* If we fail to write start of cross-linked chain test case
                           in our logger then break loop and return error. */
                        if(ret_val != NU_SUCCESS)
                        {
                            break;
                        }

                        /* Reset our error numbers. */
                        error_fl_num = error_dir_num = 0;                        
                        NUF_Copybuff(&err_dir_name[0],CHK_FOLDER_FOR_CROSS_LINK_ENTRIES,NUF_Get_Str_Len(CHK_FOLDER_FOR_CROSS_LINK_ENTRIES));
                        err_dir_name[NUF_Get_Str_Len(CHK_FOLDER_FOR_CROSS_LINK_ENTRIES)] = '\0';
                    
                    }                     
                    chk_log_add_error(CHKDSK_LOGID_CROSS_LINKED);

                    /* Check to see if we are suppose to fix the errors */
                    if(g_chkdsk_mode == CHK_FIX_ERRORS)
                    {   
                        /* Release our drive lock so we can do high level file operations. */
                        PC_DRIVE_EXIT(pdir_rec_obj->dh)

                        /* Create an error folder for the cross-link cluster chain entries. */                   
                        if((error_fl_num == 0) && (error_dir_num == 0))
                        { 
                            /* Make sure we are in the root directory. */
                            ret_val = NU_Set_Current_Dir("\\");
                            if(ret_val == NU_SUCCESS)
                            {                                
                                ret_val = chk_make_err_dir(&err_dir_name[0]);
                                if(ret_val == NU_SUCCESS)
                                {
                                    ret_val = NU_Set_Attributes(&err_dir_name[0], ADIRENT | ASYSTEM | AHIDDEN);                            
                                    /* If error found break loop and report error. */
                                    if(ret_val != NU_SUCCESS)
                                    {
                                        break;
                                    }
                                }
                            }                                                 
                        }                

                        /* Set the drive to the folder that holds the cluster chains 
                           that make up the cross-link chain. */                
                        ret_val = NU_Set_Current_Dir(&err_dir_name[0]);
                        
                        /* Reacquire our drive lock. */
                        PC_DRIVE_ENTER(pdir_rec_obj->dh,YES)  

                        if(ret_val == NU_SUCCESS)
                        {
                            /* If it is a directory. */
                            if(pdir_rec_obj->pdos_rec->fattribute & ADIRENT)
                            {                                
                                /* Read in the sectors' directory record contents. */
                                PC_DRIVE_IO_ENTER(pdir_rec_obj->dh)
                                ret_val = fs_dev_io_proc(pdir_rec_obj->dh, pc_cl2sector(pfs_cb->ddrive, cl), &buff[0], 1, YES);
                                PC_DRIVE_IO_EXIT(pdir_rec_obj->dh)                                                                    
                                
                                if(ret_val == NU_SUCCESS)
                                {
									/* Get next directory record. */
                                    DOSINODE *pnxt_dir_rec = (DOSINODE*)&buff[32];

									/* Get current directory record. */
                                    pnew_dir_rec = (DOSINODE*)&buff[0];                                     
                                    
                                    if((pc_isdot(pnew_dir_rec->fname, pnew_dir_rec->fext) == NU_TRUE)
                                        && (pc_isdotdot(pnxt_dir_rec->fname, pnxt_dir_rec->fext) == NU_TRUE))
                                    {                                            
                                        ret_val = chk_create_error_record(pdir_rec_obj->dh, pfs_cb->ddrive, &err_dir_name[0],
                                                                             CHK_DIR_CROSS_LINK_NAME, error_dir_num++, cl, 0, NU_FALSE); 
                                        if(ret_val == NU_SUCCESS)
										{
											ret_val = pc_fndnode(pdir_rec_obj->dh, &pmom, (UINT8*)&err_dir_name[0]);
											if(ret_val == NU_SUCCESS)
											{
												/* Make sure dot dot entry gets updated. */
												pnew_dir_rec->fclusterhigh = (UINT16)(pmom->finode->fcluster & 0xFFFF0000);
												pnew_dir_rec->fclusterlow = (UINT16)(pmom->finode->fcluster & 0x0000FFFF);          
	                                            
												PC_DRIVE_IO_ENTER(pdir_rec_obj->dh)
												ret_val = fs_dev_io_proc(pdir_rec_obj->dh, pc_cl2sector(pfs_cb->ddrive, cl), &buff[0], 1, NO);
												PC_DRIVE_IO_EXIT(pdir_rec_obj->dh)

												/* Make sure we let mom go. */
												pc_freeobj(pmom);                                            
											}
										}
                                    }
                                    else
                                    {
                                        /* If there isn't a dot and dot dot entry, then this cross link
                                           for the directory is occurring the in the middle of a directory
                                           listing, so store results in a file instead of a directory. */
                                   
                                        ret_val = chk_calc_file_size(&fl_size, pdir_rec_obj->dh, cl);
                                        if(fl_size < pfs_cb->ddrive->bytespcluster)
                                        {
                                            fl_size = pfs_cb->ddrive->bytespcluster;
                                             /* Create a file that contains cl until eoc_marker. */
                                            ret_val = chk_create_error_record(pdir_rec_obj->dh, pfs_cb->ddrive, &err_dir_name[0], 
                                                                    CHK_FILE_CROSS_LINK_NAME, error_fl_num++, cl, fl_size, NU_TRUE);
                                        }
                                    }
                                }
                                
                            }
                            /* It is a file. */
                            else
                            {
                                fl_size = pdir_rec_obj->pdos_rec->fsize;
								 /* If the first cluster isn't the start of the cross-link chain,
                                    then subtract the number of clusters we have traversed. */
                                if(num_cl_trav)
                                {
                                    fl_size -= (num_cl_trav *(pfs_cb->ddrive->bytspsector * pfs_cb->ddrive->secpalloc));
                                }                              
                
                                /* Create a file that contains cl until eoc_marker. */
                                ret_val = chk_create_error_record(pdir_rec_obj->dh, pfs_cb->ddrive, &err_dir_name[0], 
                                                                    CHK_FILE_CROSS_LINK_NAME, error_fl_num++, cl, fl_size, NU_TRUE);                            
                            }
                        }
                    
                    }
                
                    if(ret_val == NU_SUCCESS)
                    {
                        /* Set the cross-linked cluster chain as being resolved, even if we didn't fix it.
                           This is done so that the logger will only count the number of cross-linked chains
                           and not the number of cross-linked clusters. */
                        ret_val = chk_bitmap_mark_cl_chain(cl, pfs_cb->ddrive, CL_CHAIN_RESOLVED);
                    }        
                }
    
                  /* Delete files whose start clusters is/or part of a cross-link cluster chain.
                     However, skip this if the user doesn't want us to fix the errors. */
                if((g_chkdsk_mode == CHK_FIX_ERRORS) && (ret_val == NU_SUCCESS) && (chk_bitmap_get_value(cl) == CL_CHAIN_RESOLVED))
                {                
                    /* If prev_cl is zero then the first cluster was the start of a
                       cross-link cluster chain, so no need to update the current
                       directory, it will be deleted. */
                    if(prev_cl)
                    {
                        /* If it is a file make sure we update the directory records size and
                           free the rest of the cluster chain. */
                        if(!(pdir_rec_obj->pdos_rec->fattribute & ADIRENT))
                        {
                            /* Read in the sector that contains this directory record. */
                            PC_DRIVE_IO_ENTER(pdir_rec_obj->dh)
                            ret_val = fs_dev_io_proc(pdir_rec_obj->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, YES);
                            PC_DRIVE_IO_EXIT(pdir_rec_obj->dh)
                        
                            if(ret_val == NU_SUCCESS)
                            {
                                /* Set the new file size. */
                                pnew_dir_rec  = (DOSINODE*)&buff[pdir_rec_obj->pdos_rec_loc->offset];                
                                pnew_dir_rec->fsize =  num_cl_trav * (pfs_cb->ddrive->bytspsector * pfs_cb->ddrive->secpalloc);

                                /* Now write it back out to disk. */
                                PC_DRIVE_IO_ENTER(pdir_rec_obj->dh)
                                ret_val = fs_dev_io_proc(pdir_rec_obj->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, NO);
                                PC_DRIVE_IO_EXIT(pdir_rec_obj->dh) 
                            }
                        
                        }
                    
                        /* Adjust the FAT table accounting for both file's and directories. */
                        if(ret_val == NU_SUCCESS)
                        {
                            /* Mark the original files last cluster as EOC. 
                               NOTE: This function will handle FAT12/16/32 so just pass
                               in FAT32. */
                            ret_val = pc_pfaxx(pfs_cb->ddrive, prev_cl, (CHK_FAT32_CL_MASK & CHK_EOC));

                            if(ret_val == NU_SUCCESS)
                            {
                                PC_FAT_ENTER(pdir_rec_obj->dh)
                                ret_val = pc_flushfat(pfs_cb->ddrive);
                                PC_FAT_EXIT(pdir_rec_obj->dh)
                            }
                        }
                
                    }
                    /* First cluster from directory record was start of cross-link
                       cluster chain, so delete it. */
                    else
                    {  

                        /* Does this directory/file have a LFN record?*/
                        if(pdir_rec_obj->lfn_start_loc.cl_num)
                        {
                            /* Make the LFN records as deleted. */
                            ret_val = chk_lfn_traverse_records(pdir_rec_obj, chk_lfn_del_records, 0);                        
                        }

                        if(ret_val == NU_SUCCESS)
                        {
                            /* Read in the sector that contains this directory record. */
                            PC_DRIVE_IO_ENTER(pdir_rec_obj->dh)
                            ret_val = fs_dev_io_proc(pdir_rec_obj->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, YES);
                            PC_DRIVE_IO_EXIT(pdir_rec_obj->dh)

                            if(ret_val == NU_SUCCESS)
                            {
                                /* Mark the file as deleted. */
                                pnew_dir_rec  = (DOSINODE*)&buff[pdir_rec_obj->pdos_rec_loc->offset];                    
                                pnew_dir_rec->fname[0] = PCDELETE;

                                /* Now write it back out to disk. */
                                PC_DRIVE_IO_ENTER(pdir_rec_obj->dh)
                                ret_val = fs_dev_io_proc(pdir_rec_obj->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, NO);
                                PC_DRIVE_IO_EXIT(pdir_rec_obj->dh)                               
                            }
                        }
                
                    }
                
                    if(ret_val == NU_SUCCESS)
                    {
                        /* Set the update flag in the dir_rec_obj, so the traversal 
                           function(chk_traverse_all_dir_paths) knows the disk has been updated. */
                        pdir_rec_obj->disk_updated = NU_TRUE;
                    }        
                }

                if(ret_val == NU_SUCCESS)
                {
                    prev_cl = cl;
                    ret_val = pc_clnext(&cl, pfs_cb->ddrive, cl);    
                    ++num_cl_trav;                    
                }

            } while(cl && (pdir_rec_obj->disk_updated != NU_TRUE) && (ret_val == NU_SUCCESS));            

            if(ret_val == NU_SUCCESS)
            {
                /* Release our drive lock so we can do high level file operations. */
                PC_DRIVE_EXIT(pdir_rec_obj->dh)

                /* Set our current directory back to root. */
                ret_val = NU_Set_Current_Dir("\\");

                /* Reacquire our drive lock. */
                PC_DRIVE_ENTER(pdir_rec_obj->dh,YES)  
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
*   chk_create_error_record
*
* DESCRIPTION
*
*   Create a file or directory from the parameters passed in. 
*   This function is used to create the error files/directories
*   for the lost cluster chains and cross-linked chains.
*          
*
* INPUTS
*  
*   dh(in)                      Pointer to a Disk Handle.
*   pddrive(in)                 Pointer to DDRIVE Structure.
*   pparent_folder(in)          The string name of the directory
*                               where the file/directory that is 
*                               being created should go.
*   perr_name(in)               The string name of the error 
*                               file/directory to create.
*   err_cnt(in)                 The number added to the err_name
*                               to make it unique.
*   cl(in)                      The cluster that this directory
*                               record should start with.
*   dir_rec_size(in)            The size value for the directory
*                               record.
*   b_is_fl(in)                 Boolean indicating whether this is
*                               a file or a directory.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Error record was created and associated
*                               with the cluster value passed in.
*   NUF_BADPARM                 One or more parameters are bad.                  
*
*************************************************************************/
STATUS chk_create_error_record(UINT16 dh, DDRIVE *pddrive, CHAR *pparent_folder, CHAR *perr_name,
                               UINT16 err_cnt, UINT32 cl, UINT32 dir_rec_size, UINT8 b_is_fl)
{
    STATUS ret_val;
    CHAR err_dir_rec_name[12] = {'\0'};     
    DROBJ *pmom = NU_NULL;
    DROBJ *pdobj = NU_NULL; 
   
    if((pparent_folder != NU_NULL) && (perr_name != NU_NULL))
    {
        ret_val = pc_fndnode(dh, &pmom, (UINT8*)pparent_folder);
        if(ret_val == NU_SUCCESS)
        {			        
            ret_val = chk_create_error_name(&err_dir_rec_name[0], perr_name, err_cnt, NU_FALSE);
            if(ret_val == NU_SUCCESS)
            {        
                ret_val = pc_mknode(&pdobj, pmom, (UINT8*)&err_dir_rec_name[0], (UINT8*)CHK_FILE_ERROR_EXTENSION,
                          (UINT8)(b_is_fl ? ARCHIVE : ADIRENT));

                if(ret_val == NU_SUCCESS)
                {
                    /* If we created a new directory for contents, 
                       clear out cluster allocated because we are going to replace cluster
                       value. */
                    if(b_is_fl == NU_FALSE)
                    {                    
                        ret_val = pc_clrelease(pddrive, pdobj->finode->fcluster);
                    }
                    if(ret_val == NU_SUCCESS)
					{
						pdobj->finode->fcluster = cl;
						pdobj->finode->fsize = dir_rec_size;

						ret_val = pc_update_inode(pdobj, DSET_ACCESS);
						if(ret_val == NU_SUCCESS)
						{   
							/* Make sure all our buffers are free. */
							pc_free_all_blk(pddrive);                       
						}
					}
               }
               pc_freeobj(pdobj);
           }       
           pc_freeobj(pmom);
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
*   chk_fat_table_compare_test
*
* DESCRIPTION
*
*   This function verifies that the two FAT tables match.        
*          
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  FAT Tables are the same or where
*                               resolved to be the same. 
*   NUF_FAT_TABLES_DIFFER       FAT Tables differ, and weren't fixed.                 
*
*************************************************************************/
STATUS chk_fat_table_compare_test(UINT16 dh)
{
    STATUS ret_val;
    FAT_TABLE_TRAV_BLK fat_table1_data, fat_table2_data;
    UINT16 i;
    UINT8 b_fat_tables_differ = NU_FALSE; 
   
    /* If we are fixing errors, then it is okay to create a log file. */
    if(g_chkdsk_mode == CHK_FIX_ERRORS)
    {        
        /* Make sure our global list of log records gets cleared,
           Need to clear it before each test to make room for
           current test possible errors. */
        CHK_CLEAR_LOG_REC();           
    }

    PC_DRIVE_ENTER(dh,YES)
    ret_val = chk_get_first_fat_trav_blk(&fat_table1_data, dh, 1);
    if(ret_val == NU_SUCCESS)
    {
        ret_val = chk_get_first_fat_trav_blk(&fat_table2_data, dh, 2);
        if(ret_val == NU_SUCCESS)
        {

            /* Loop until we have compared both FAT tables. */
	        do 
            {
                /* Check each byte in the sector that was read in to make sure they are equal. */
		        for(i = 0; i < CHKDSK_SECTOR_SIZE;++i)
		        {
			        if(fat_table1_data.data[i] != fat_table2_data.data[i])
			        {				        
                        b_fat_tables_differ = NU_TRUE;
                        ret_val = NUF_FAT_TABLES_DIFFER;
                        break;
			        }
		        }
		        
                /* Make sure the FAT tables are still the same. */
                if(b_fat_tables_differ == NU_FALSE)
                {
                    /* If we have compared all the data in the FAT table then exit the loop. */
                    if(fat_table1_data.current_fat_sector == fat_table1_data.last_fat_sector && 
                       fat_table2_data.current_fat_sector == fat_table2_data.last_fat_sector)
                    {
                        break;
                    }
                    else                    
                    {
                        /* Read in the next sector of the first FAT table. */
                        ret_val = chk_get_next_fat_trav_blk(&fat_table1_data);
		                if(ret_val == NU_SUCCESS)                
                        {
                            /* Read in the next sector of the second FAT table. */
                            ret_val= chk_get_next_fat_trav_blk(&fat_table2_data); 
                        }
                    }
                }

	        } while(b_fat_tables_differ == NU_FALSE);
        }
    }
    PC_DRIVE_EXIT(dh)

    /* If the fat tables differ then copy the first fat table to
       the second one. */    
    if(b_fat_tables_differ == NU_TRUE)
    {
        /* Are we fixing errors or just reporting them. */
        if(g_chkdsk_mode == CHK_FIX_ERRORS)
        {
            /* Start the logger. */
            ret_val = chk_start_test_case(CHK_CHECK_FAT_TABLES);
            if(ret_val == NU_SUCCESS)
            {
                PC_DRIVE_ENTER(dh,YES)
                ret_val = chk_get_first_fat_trav_blk(&fat_table1_data, dh, 1);
                if(ret_val == NU_SUCCESS)
                {
                    ret_val = chk_get_first_fat_trav_blk(&fat_table2_data, dh, 2);                   
                    /* Traverse FAT table 1. */
                    while((fat_table1_data.current_fat_sector != fat_table1_data.last_fat_sector) 
                          && (ret_val == NU_SUCCESS))
                    {
                        /* Now write out FAT table 2. */
                        PC_DRIVE_IO_ENTER(dh)
                        ret_val = fs_dev_io_proc(dh, (UINT32)(fat_table2_data.current_fat_sector), &(fat_table1_data.data[0]), 1, NO);                        
                        PC_DRIVE_IO_EXIT(dh)
                        if(ret_val == NU_SUCCESS)
                        {
                            /* Read in the next sector of the first FAT table. */
                            ret_val = chk_get_next_fat_trav_blk(&fat_table1_data);
		                    if(ret_val == NU_SUCCESS)                
                            {
                                /* Read in the next sector of the second FAT table. */
                                ret_val= chk_get_next_fat_trav_blk(&fat_table2_data); 
                            }
                        }
                    }                 
                }
                PC_DRIVE_EXIT(dh)
            
                /* Change our error value to NU_SUCCESS, because the errors where resolved. */
                if(ret_val == NUF_FAT_TABLES_DIFFER)
                {
                    ret_val = NU_SUCCESS;
                }

                if(ret_val == NU_SUCCESS)
                {
                    /* Report errors to our log. */
                    ret_val = chk_write_mess(FAT_TABLE_ERR_MESS, 1); 
                    /* Write our our end tag for this case. */
                    if(ret_val == NU_SUCCESS)
                    {
                       ret_val = chk_end_test_case();
                    }            
                }
            }
        }            
    }        
    
    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_lfn_traverse_records
*
* DESCRIPTION
*
*   This function will traverse a LFN, one directory record at a time. 
*   For each directory record the function pointer(lfn_record_op) will 
*   be called so that its operation can be preformed.
*       
*          
*
* INPUTS
*  
*   *pdir_rec_obj(in)           Directory Record Object.
*   *lfn_record_op(in)          Pointer to a function
*                               that is to be called for 
*                               for every LFN directory record.
*   sfn_chksum(in)              The SFN's checksum for which
*                               this LFN is associated with.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  LFN directory records where
*                               traversed successfully.
*   NUF_BADPARM                 One or more parameters are bad. 
*
*************************************************************************/
STATUS chk_lfn_traverse_records(DIR_REC_OBJ *pdir_rec_obj, STATUS (*plfn_record_op)(UINT8 *pupdate, UINT8 *plfn_record, UINT8 sfn_chksum), 
                                UINT8 sfn_chksum)
{
    STATUS ret_val;
    UINT32 curr_cl;
    UINT32 curr_sec;
    UINT32 sec_offset = 1;
    UINT32 end_sec;
    UINT16 curr_buff_offset;
    UINT8 buff[CHKDSK_SECTOR_SIZE];
    UINT8 exit_all_loops = NU_FALSE;    
    UINT8 update_disk = NU_FALSE;
    DIR_REC_LOC *lfn_loc = &(pdir_rec_obj->lfn_start_loc);
    DOSINODE *dir_rec;    
    DDRIVE *pddrive;
    FAT_CB *pfs_cb = NU_NULL;

    if((pdir_rec_obj != NU_NULL) && (plfn_record_op != NU_NULL))
    {    
        ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
        if(ret_val == NU_SUCCESS)
        {
            pddrive = pfs_cb->ddrive;
            /* Setup our traverse starting positions and offsets. */
            curr_cl = lfn_loc->cl_num;
            curr_sec = lfn_loc->sec_num;
            curr_buff_offset = lfn_loc->offset;

            /* Because FAT12/16 root directory is static we have to calculate an end sector. */
            if(lfn_loc->fat_12_16_root == NU_TRUE)
            {
                sec_offset = curr_sec;
                end_sec = ((pddrive->numroot * 32)/CHKDSK_SECTOR_SIZE) + pddrive->rootblock;
            }
            /* FAT32 */
            else
            {
                /* Get our sector offset. */
                sec_offset += pc_sec2index(pddrive, curr_sec);
                end_sec = pddrive->secpalloc;
            }

            /* Traverse the cluster list. */
            /* Fist condition in while loop is accounting for the two different situations we could run into,
               The first situation being we are traversing a cluster chain. The second being we are traversing
               a FAT12/16 root directory. */
            while((curr_cl || (lfn_loc->fat_12_16_root == NU_TRUE)) && (ret_val == NU_SUCCESS) && (exit_all_loops == NU_FALSE))
            {
                /* No point in calling this function if we are traversing,
                   a static block of sectors(root directory FAT12/16). */
                if(lfn_loc->fat_12_16_root != NU_TRUE)
                {
                    ret_val = pc_clnext(&curr_cl, pddrive, curr_cl);
                    if(ret_val != NU_SUCCESS)
                    {                        
                        break;                        
                    }
                }
               
               /* Traverse sectors */
                do 
                {        
                    PC_DRIVE_IO_ENTER(pddrive->dh)
                    ret_val = fs_dev_io_proc(pddrive->dh, curr_sec, &buff[0], 1, YES);
                    PC_DRIVE_IO_EXIT(pddrive->dh)

                    if(ret_val == NU_SUCCESS)
                    {            
                        /* Traverse buffer */
                        do 
                        {                             
                            dir_rec = (DOSINODE *) &buff[curr_buff_offset];
                            if(dir_rec->fattribute != CHK_LFN_ATTR)
                            {
                                exit_all_loops = NU_TRUE;
                                break;
                            }
                            else
                            {
                                /* Call function pointer. */
                                ret_val = plfn_record_op(&update_disk, &buff[curr_buff_offset], sfn_chksum);
                                if(ret_val != NU_SUCCESS)
                                {
                                    exit_all_loops = NU_TRUE;
                                }
                            }

                	        curr_buff_offset += 32;

                        } while((curr_buff_offset < CHKDSK_SECTOR_SIZE) && 
                                (exit_all_loops == NU_FALSE));
                    }
                  
                    /* See if we need to update disk. */
                    if((update_disk == NU_TRUE) && (ret_val == NU_SUCCESS))
                    {
                        PC_DRIVE_IO_ENTER(pddrive->dh)
                        ret_val = fs_dev_io_proc(pddrive->dh, curr_sec, &buff[0], 1, NO);
                        PC_DRIVE_IO_EXIT(pddrive->dh)
                        update_disk = NU_FALSE;
                        pdir_rec_obj->disk_updated = NU_TRUE;
                    }
            
                    ++sec_offset;
                    ++curr_sec;
                    curr_buff_offset = 0;
                } while((sec_offset <= end_sec) && (exit_all_loops == NU_FALSE) && (ret_val == NU_SUCCESS));

                if(ret_val == NU_SUCCESS)
                {
                    /* If this isn't a FAT12/16 root directory convert cluster to sector. */    
                    if(lfn_loc->fat_12_16_root != NU_TRUE)
                    {        
                        /* Get our new sector number. */
                        curr_sec = pc_cl2sector(pddrive, curr_cl);
                    }

                    /* Reset our sector offset. */
                        sec_offset = 1;
                }    
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
*   chk_lfn_del_records
*
* DESCRIPTION
*
*   This function sets the LFN directory record passed in as
*   being deleted. 
*        
*          
*
* INPUTS
*  
*   *pupdate(out)               Flag to indicate to the disk was updated.
*   *plfn_record(in)            A pointer to a LFN directory record entry.
*   sfn_checksum(in)            This LFN's directory records SFN's
*                               checksum.            
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  LFN directory record was
*                               marked as deleted. 
*   NUF_BADPARM                 One or more of the parameters
*                               are bad.                 
*
*************************************************************************/
STATUS chk_lfn_del_records(UINT8 *pupdate, UINT8 *plfn_record, UINT8 sfn_chksum)
{
    STATUS ret_val = NU_SUCCESS;   
    
    FILE_Unused_Param = sfn_chksum; /* Resolve compiler warning. */

    if(plfn_record != NU_NULL)
    {
        plfn_record[0] = PCDELETE;
        *pupdate = NU_TRUE;
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
*   chk_lfn_check
*
* DESCRIPTION
*
*   This function will check a LFN name for the following:
*       -Its character count doesn't exceed the max, if
*        it does an error is reported.
*       -The LFN checksum is correct, if it isn't an
*        error is reported.
*       -The LFN directory record's cluster is zero, if it isn't
*        it is set to zero and an error is reported.
*        
*          
*
* INPUTS
*  
*   *pupdate(out)               Flag set by the function to indicate
*                               the disk has been updated.
*   *plfn_record(in)            Pointer to the start of a LFN directory record.
*   sfn_chksum(in)              The checksum that the LFN directory
*                               entries should match.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  LFN directory records where checked.
*   NUF_BADPARM                 One or more of the parameters
*                               are bad.    
*
*************************************************************************/
STATUS chk_lfn_check(UINT8 *pupdate, UINT8 *plfn_record, UINT8 sfn_chksum)
{
    STATUS ret_val = NU_SUCCESS;    
    static UINT16 char_count = 0;
    UINT8 b_reset_char_cnt = NU_FALSE;
    UINT8 i;    

    if(plfn_record != NU_NULL)
    {
        /* Check the checksum. */
        if(plfn_record[13] != sfn_chksum)
        {
            /* Log error. */
            chk_log_add_error(CHKDSK_LOGID_DDR_LFN_CHKSUM);
        }

        /* Make sure LFN cluster entry is zero, if it isn't set it to zero
           and log the error. */
        if((plfn_record[26] != 0x00) || 
           (plfn_record[27] != 0x00))
        {
            /* Log errors. */            
            chk_log_add_error(CHKDSK_LOGID_DDR_LFN_CL);
        
            if(g_chkdsk_mode == CHK_FIX_ERRORS)
            {
                plfn_record[26] = plfn_record[27] = 0x00;
                *pupdate = NU_TRUE;
            }
        
        }
    

        /* Check to see how many characters are in this LFN
           directory record. */ 

        /* Check last one before we check the rest. */
        if((plfn_record[30] != 0xFF) && (plfn_record[31] != 0xFF))
        {            
            char_count+=13;
        }
        /* Last Unicode character is 0xFFFF so we have found our last
           LFN directory record. */
        else
        {
            /* Check characters 1-5. */
            for(i = 1; i < 10; i+=2)
            {
                if((plfn_record[i] == 0xFF) && (plfn_record[i+1] == 0xFF))
                {
                   /* Reset our character count because we found last valid LFN
                      directory record. */
                   b_reset_char_cnt = NU_TRUE;
                   break;
                }
                ++char_count;
            }

            /* Check characters 6-11 */
            for(i = 14; ((i < 26) && (b_reset_char_cnt == NU_FALSE)); i+=2)
            {
                if(plfn_record[i] == 0xFF && plfn_record[i+1] == 0xFF)
                {
                    /* Reset our character count because we found last valid LFN
                       directory record. */
                   b_reset_char_cnt = NU_TRUE;
                }
                ++char_count;
            }

            /* Check character 12. */
            if((plfn_record[28] != 0xFF) && (plfn_record[29] != 0xFF))
            {                            
                /* Add one character for character number 12. */
                ++char_count;
            }
            else
            {
                /* Reset our character count because we found last valid LFN
                   directory record. */
                b_reset_char_cnt = NU_TRUE;            
            }
        
        }

        /* Verify the LFN length. */
        if(char_count > MAX_SIZE_OF_LFN_NAME)
        {
            /* Log errors. */
           chk_log_add_error(CHKDSK_LOGID_DDR_LFN_EXCEED_MAX);
        
            b_reset_char_cnt = NU_TRUE;
        }
        else
        {
            /* Check to see if this is the first LFN directory record,
               which would be our last one because we are counting backwards. 
               if it is reset the counter. */
            if((plfn_record[0] | 0x40) == 0x41)
            {   
                char_count = 0;
            }
        }

        if(b_reset_char_cnt == NU_TRUE)
        {
            char_count = 0;
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
*   chk_check_sfn_chars
*
* DESCRIPTION
*
*   This function will check to see if the characters in the
*   SFN for the file/directory name a legal. If they aren't
*   the error will be reported, but not fixed.
*          
*
* INPUTS
*  
*   *psfn_dir_rec(in)           Pointer to a DOSINODE struct,
*                               which is a directory record.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  SFN was checked for invalid characters.
*   NUF_BADPARM                 One or more of the parameters
*                               are bad.
*
*************************************************************************/
STATUS chk_check_sfn_chars(DOSINODE *psfn_dir_rec)
{
    STATUS ret_val = NU_SUCCESS;
    UINT8 i = 0;
    UINT8 b_found_bad_char = NU_FALSE;
    
    if(psfn_dir_rec != NU_NULL)
    {
        /* Special chase for kanji character. */
        if((psfn_dir_rec->fname[i] < 0x20) && (psfn_dir_rec->fname[i] != 0x05))
        {
            b_found_bad_char = NU_TRUE;
        }

        /* Check the filename. */
        for(i = 0; i < 8; ++i)
        {
            switch(psfn_dir_rec->fname[i]) {
            case 0x22:
            case 0x2A:
            case 0x2B:
            case 0x2C:
            case 0x2E:
            case 0x2F:
            case 0x3A:
            case 0x3B:
            case 0x3C:
            case 0x3D:
            case 0x3E:
            case 0x3F:
            case 0x5B:
            case 0x5C:
            case 0x5D:
            case 0x7C:
                {
                    b_found_bad_char = NU_TRUE;
                }
        	    break;
            default:
                break;
            }
        
        
        }
        /* Check the extension. */
        for(i = 0; i < 3; ++i)
        {
            switch(psfn_dir_rec->fext[i]) {
            case 0x22:
            case 0x2A:
            case 0x2B:
            case 0x2C:
            case 0x2E:
            case 0x2F:
            case 0x3A:
            case 0x3B:
            case 0x3C:
            case 0x3D:
            case 0x3E:
            case 0x3F:
            case 0x5B:
            case 0x5C:
            case 0x5D:
            case 0x7C:
                {
                    b_found_bad_char = NU_TRUE;
                }
        	    break;
            default:
                break;
            }
        }
    
        if(b_found_bad_char == NU_TRUE)
        {
            /* Log errors. */
           chk_log_add_error(CHKDSK_LOGID_DDR_SFN_ILLEGAL_CHAR);
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
*   chk_check_dir
*
* DESCRIPTION
*
*   This function will check for the following issues:
*       -Verify that the directories directory record size is zero.
*        If it isn't the function will change it to zero, update
*        the disk, and log the error.
*       -Verify that the dot's directory record cluster for the directory 
*        in dir_rec_obj points to itself. If it doesn't the dot's cluster 
*        will be fixed and the error will be logged.
*       -Verify that the dot dot's directory record cluster for the directory
*        in dir_rec_obj points to the parent directories cluster. If it 
*       doesn't the dot dot's cluster will be fixed and the error logged.
*          
*
* INPUTS
*  
*   *pdir_rec_obj(in)           Directory Record Object.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  Directory was checked for errors.
*   NUF_BADPARM                 One or more of the parameters
*                               are bad.
*
*************************************************************************/
STATUS chk_check_dir(DIR_REC_OBJ *pdir_rec_obj)
{
    STATUS ret_val;
    UINT32 cl,parent_cl;
    UINT32 dots_cl;
    UINT32 sec;    
    UINT8 buff[CHKDSK_SECTOR_SIZE];
    UINT8 b_update_disk = NU_FALSE;
    DOSINODE *pdos_node;    
    DDRIVE *pddrive;
    FAT_CB *pfs_cb;
    
    if(pdir_rec_obj != NU_NULL)
    {    
        ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
        if(ret_val == NU_SUCCESS)
        {
            pddrive = pfs_cb->ddrive;
            pdos_node = pdir_rec_obj->pdos_rec;    
                
            /* Make sure the directory size in the directory record is zero. */
            if(pdos_node->fsize != 0)
            {   
                /* Log errors. */
                chk_log_add_error(CHKDSK_LOGID_DDR_DIR_SIZE);
        
                /* Make sure we are suppose to fix the errors. */
                if(g_chkdsk_mode == CHK_FIX_ERRORS)
                {
            
                    PC_DRIVE_IO_ENTER(pddrive->dh)
                    ret_val = fs_dev_io_proc(pddrive->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, YES);
                    PC_DRIVE_IO_EXIT(pddrive->dh)
                
                    if(ret_val == NU_SUCCESS)
                    {                
                        /* Zero out the size. */
                        pdos_node = (DOSINODE*)&buff[pdir_rec_obj->pdos_rec_loc->offset];
                        pdos_node->fsize = 0x00;

                        /* Make sure the traversal function knows the current buffer has changed on the disk. */
                        pdir_rec_obj->disk_updated = NU_TRUE;   

                        /* Write this back out to the disk. */
                        PC_DRIVE_IO_ENTER(pddrive->dh)
                        ret_val = fs_dev_io_proc(pddrive->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, NO);
                        PC_DRIVE_IO_EXIT(pddrive->dh)
                    }
                }
        
            }  
            
            if(ret_val == NU_SUCCESS)
            {
                    cl = chk_get_dir_rec_cl(pddrive, pdos_node);

                /* Get the parent cluster value so we can check dot dot. 
                   The current directories dot dot cl value should be the same
                   as the dot dot's directories dot cluster value. */            
            
                /* First dot dot's cluster value. */
                sec = pc_cl2sector(pddrive, cl);
                PC_DRIVE_IO_ENTER(pddrive->dh)
                ret_val = fs_dev_io_proc(pddrive->dh, sec, &buff[0], 1, YES);
                PC_DRIVE_IO_EXIT(pddrive->dh)
            
                pdos_node = (DOSINODE*)&buff[32];
                parent_cl = chk_get_dir_rec_cl(pddrive,pdos_node);
                sec = pc_cl2sector(pddrive,parent_cl);

                /* If the parent directory isn't the root directory, then
                   read in the parents dot cluster value. Note if a directories
                   dot dot cluster value is zero than its parent
                   directory is the root directory. */
                if(parent_cl != 0)
                {
                    PC_DRIVE_IO_ENTER(pddrive->dh)
                    ret_val = fs_dev_io_proc(pddrive->dh, sec, &buff[0], 1, YES);
                    PC_DRIVE_IO_EXIT(pddrive->dh)

                    pdos_node = (DOSINODE*)&buff[0];
                    parent_cl = chk_get_dir_rec_cl(pddrive,pdos_node);
                }

                /* Check the following:
                   dot, and dot dot point to correct clusters. */
                if(ret_val == NU_SUCCESS)
                {
                    sec = pc_cl2sector(pddrive, cl);
                    PC_DRIVE_IO_ENTER(pddrive->dh)
                    ret_val = fs_dev_io_proc(pddrive->dh, sec, &buff[0], 1, YES);
                    PC_DRIVE_IO_EXIT(pddrive->dh)

                    if(ret_val == NU_SUCCESS)
                    {
            
                        /* Check the dot. */
                        pdos_node = (DOSINODE*)&buff[0];
                    
                        dots_cl = chk_get_dir_rec_cl(pddrive,pdos_node);
                    
                        if(dots_cl != cl)
                        {
                            /* Log errors. */
                           chk_log_add_error(CHKDSK_LOGID_DDR_DOT);                

                            /* Make sure we are suppose to fix the errors. */
                            if(g_chkdsk_mode == CHK_FIX_ERRORS)
                            {                
                                /* Indicate that disk was changed. */
                                b_update_disk = NU_TRUE;

                                dots_cl = cl;
        
                                /* Write the new cluster value to the buffer. */
                                if(CHK_IS_DRIVE_FAT32(pddrive))
                                {
                                    pdos_node->fclusterhigh = (UINT16)((dots_cl & 0xFFFF0000) >> 16);
                                    pdos_node->fclusterlow = (UINT16)dots_cl;               
                                }
                                else
                                {   /* FAT12/16 */                
                                    pdos_node->fclusterlow = (UINT16)dots_cl;    
                                }
                            }
        
                        }
            

                        /* Check the dot dot. */
                        ++pdos_node;
                    
                        dots_cl = chk_get_dir_rec_cl(pddrive,pdos_node);                    
                    
                        if(dots_cl != parent_cl || parent_cl == cl)
                        {
                            /* Log errors. */
                            chk_log_add_error(CHKDSK_LOGID_DDR_DOT_DOT);
        
                            /* Make sure we are suppose to fix the errors. */
                            if(g_chkdsk_mode == CHK_FIX_ERRORS)
                            {
                                /* Indicate that disk was changed. */
                                b_update_disk = NU_TRUE;

                                dots_cl = pdir_rec_obj->dos_rec_moms_loc.cl_num;

                                 /* Write the new cluster value to the buffer. */
                                if(CHK_IS_DRIVE_FAT32(pddrive))
                                {
                                    pdos_node->fclusterhigh = (UINT16)((dots_cl & 0xFFFF0000) >> 16);
                                    pdos_node->fclusterlow = (UINT16)dots_cl;
                                }
                                else
                                {   /* FAT12/16 */                
                                    pdos_node->fclusterlow = (UINT16)dots_cl;
    
                                }
                            }        
                        }
    
                        /* Buffer data was changed so update the disk. */
                        if(b_update_disk == NU_TRUE)
                        {

                            PC_DRIVE_IO_ENTER(pddrive->dh)
                            ret_val = fs_dev_io_proc(pddrive->dh, sec, &buff[0], 1, NO);
                            PC_DRIVE_IO_EXIT(pddrive->dh)

                            if(ret_val == NU_SUCCESS)
                            {
                                /* Make sure the traversal function knows the current buffer has changed on the disk. */
                                pdir_rec_obj->disk_updated = NU_TRUE;                
                            }
                        }
                    }      
                }
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
*   chk_is_cl_valid
*
* DESCRIPTION
*
*   This function will take the cluster passed in and verify
*   whether it is valid or not.
*
*   Values that are checked:
*       (NOTE: ? used to indicate that for other versions of FAT
*              there will be more hex digits.)
*       -Free       0x0
*       -Reserved   0x?FF0 - 0x?FF6, 0x1     
*       -Bad        0x?FF7
*       -Cluster is out of range of valid clusters on the drive.  
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value from a directory
*                               record.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  cl is a valid cluster.
*   NUF_CHK_CL_INVALID          cl is an invalid cluster value.
*   NUF_BADPARM                 pddrive is NU_NULL.
*
*************************************************************************/
STATUS chk_is_cl_valid(DDRIVE *pddrive, UINT32 cl)
{
    STATUS ret_val = NU_SUCCESS;
    UINT32 cl_mask;

    if(pddrive != NU_NULL)
    {
        /* Determine our FAT cluster mask. */    
        cl_mask = chk_get_cl_mask(pddrive);

        /* If the cluster value isn't an eoc marker check its value,
           use < because 0x?FF8 - 0xFFF are all valid EOC and we
           only want to check clusters values that are less than this
           to make sure they are valid. */
          
        if(cl < (cl_mask & CHK_EOC))
        {      
            /* Check to see if cluster is bad. 
               (cl_mask & CHK_BAD_CL) = 0x?FF7 */
            if(chk_is_cl_bad(pddrive, cl) == NU_TRUE)
            {
                ret_val = NUF_CHK_CL_INVALID;                          
            }
            else
            {            
                if(chk_is_cl_res(pddrive, cl) == NU_TRUE)
                {
                    ret_val = NUF_CHK_CL_INVALID;
                }
                else
                {               
                    /* Make sure the cluster value doesn't indicate its free. */
                    if(chk_is_cl_free(pddrive, cl) == NU_TRUE)
                    {
                        ret_val = NUF_CHK_CL_INVALID;
                    }
                    else
                    {
                        /* Make sure the cluster is in range.
                          (pddrive->numsecs /  pddrive->secpalloc) is equal to the number of clusters 
                          in the system. */
                        if(chk_is_cl_range(pddrive, cl) == NU_TRUE)
                        {
                            ret_val = NUF_CHK_CL_INVALID;
                        }
                    }
                }
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
*   chk_is_cl_value_valid
*
* DESCRIPTION
*
*   This function will take the cluster passed in and get its
*   FAT table value. It will the check to ensure that the 
*   cluster value(one read from FAT) is valid. 
*
*   Values that are checked:
*       (NOTE: ? used to indicate that for other versions of FAT
*              there will be more hex digits.)
*       -Free       0x0
*       -Reserved   0x?FF0 - 0x?FF6, 0x1     
*       -Bad        0x?FF7
*       -Cluster is out of range of valid clusters on the drive.  
*  
*          
*
* INPUTS
*  
*   *pddrive(in)                Drive Pointer.
*   cl(in)                      Cluster value from a directory
*                               record.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  cl is a valid cluster.
*   NUF_CHK_CL_INVALID          cl is an invalid cluster value.
*   NUF_BADPARM                 pddrive is NU_NULL.
*
*************************************************************************/
STATUS chk_is_cl_value_valid(DDRIVE *pddrive, UINT32 cl)
{
    STATUS ret_val = NU_SUCCESS;
    UINT32 cl_mask;
    UINT32 cl_value;

    if(pddrive != NU_NULL)
    {
        /* Determine our FAT cluster mask. */    
        cl_mask = chk_get_cl_mask(pddrive);
         
        /* If the cluster value isn't an eoc marker check its value,
           use < because 0x?FF8 - 0xFFF are all valid EOC and we
           only want to check clusters values that are less than this
           to make sure they are valid. */          
        if(cl < (cl_mask & CHK_EOC))
        {
            /* Get the clusters value from the FAT table. */
            ret_val = pc_faxx(pddrive, cl, &cl_value);
            
            if(ret_val == NU_SUCCESS)
            {
                ret_val = chk_is_cl_valid(pddrive, cl_value);
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
*   chk_dir_rec_entry_test
*
* DESCRIPTION
*
*   Driver function for the directory record entry test. It
*   starts the directory record's logger case, runs the 
*   directory record test, then writes out the logger error
*   messages and closes the logger test case.
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If all directory records
*                               where checked for errors.
*
*************************************************************************/
STATUS chk_dir_rec_entry_test(UINT16 dh)
{
    STATUS ret_val;
    CHKDSK_LOGID i;
    UINT8 b_tc_started = NU_FALSE;

    /* Make sure our global list of log records gets cleared,
       Need to clear it before each test to make room for
       current test possible errors. */
    CHK_CLEAR_LOG_REC();

    /* Check all cluster chains for invalid cluster values. */
    ret_val = chk_dir_rec_check_for_iv_cl_values(dh);
    if(ret_val == NU_SUCCESS)
    {
        PC_DRIVE_ENTER(dh,YES)
        /* Execute our test. */
        ret_val = chk_traverse_all_dir_paths(dh, chk_dir_rec_entry_test_op, NU_FALSE, NU_FALSE);            
        PC_DRIVE_EXIT(dh)
        if (ret_val == NU_SUCCESS)
        {
            /* Write out our error messages. */
            for(i = CHKDSK_LOGID_DDR_START_IDX; ((i <= CHKDSK_LOGID_DDR_END_IDX) && (ret_val == NU_SUCCESS)); ++i)
            {
                if(chk_log_is_error_num_zero(i) == NU_FALSE)
                {
                    if(b_tc_started == NU_FALSE)
                    {
                        /* Start our logger. */
                        ret_val = chk_start_test_case(CHK_CHECK_DIR_RECORDS);
                        /* If we fail to write start of directory record entry test case
                           in our logger then break loop and return error. */
                        if(ret_val != NU_SUCCESS)
                        {
                            break;
                        }
                        
                        b_tc_started = NU_TRUE;
                    }
                        
                    ret_val = chk_log_write_errors_to_file(i);
                }
            }
                 
            if((ret_val == NU_SUCCESS) && (b_tc_started == NU_TRUE))
            {
                /* End our test case in our logger. */
                ret_val = chk_end_test_case();
            }
        }
    }
    
    return ret_val;

}

/************************************************************************
* FUNCTION
*
*   chk_dir_rec_entry_test_op
*
* DESCRIPTION
*
*   This function will check a directory record entry for the following:
*       -Verify root cluster if the drive is FAT32, if FAT12/16 will verify
*        root directory length.
*       -Verify that the directory record's cluster value in the FAT
*        table is valid.
*       -Verify LFN entry doesn't exceed its max number of characters,
*        its cluster value is always zero, and that its checksum
*        is correct.
*       -If the directory record is a directory it will verify that
*        the dot and dot dot directory record entries cluster's are 
*        correct and that the directories directory record's size 
*        value is zero.
*
*  If one of the checks fails, some will only be reported, and 
*  some won't. See the specific functions that does the checks
*  to determine if the fix will be report only.
*          
*
* INPUTS
*  
*   *pdir_rec_obj(in)           Directory Record Object.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If directory record
*                               checks where successful.
*   NUF_BADPARM                 pdir_rec_obj points to NU_NULL.
*
*************************************************************************/
STATUS chk_dir_rec_entry_test_op(DIR_REC_OBJ *pdir_rec_obj)
{
    STATUS ret_val;
    UINT32 cl;        
    UINT8 sfn_chksum;        
    DDRIVE *pddrive;
    DOSINODE *pdos_node;
    FAT_CB *pfs_cb = NU_NULL;
    UINT32 cl_fat;
	UINT8 buff[CHKDSK_SECTOR_SIZE];
    

    if(pdir_rec_obj != NU_NULL)
    {
        ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
        if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
        {
            pddrive = pfs_cb->ddrive;
            pdos_node = pdir_rec_obj->pdos_rec;

            /* Check roots clusters if it is a FAT32 drive. */
            if(CHK_IS_DRIVE_FAT32(pddrive))
            {
                ret_val = pc_faxx(pddrive,pddrive->rootdirstartcl,&cl);                
                if(ret_val == NU_SUCCESS)
                {
                    ret_val= chk_is_cl_valid(pddrive, cl);
                    /* If the return value doesn't equal success check and see if our cluster
                       was invalid. */
                    if(ret_val == NUF_CHK_CL_INVALID)
                    {
                         /* Log Errors. */
                        chk_log_add_error(CHKDSK_LOGID_DDR_ROOT_START_CL);          
                    }            
                }
                
            }

            /* Verify the root directories size, this can only be done 
               for FAT12/16, because there root directory is static.*/
            if(!CHK_IS_DRIVE_FAT32(pddrive))
            {
                UINT32 start_sec = pddrive->rootblock;            
                UINT32 end_sec = start_sec + (((pddrive->numroot * 32) / CHKDSK_SECTOR_SIZE));
                         
            
                if(((end_sec - start_sec) * pddrive->bytspsector) != (UINT16)(pddrive->numroot * 32))
                {
                    chk_log_add_error(CHKDSK_LOGID_DDR_ROOT_DIR_RANGE);
                }
            }
    
            /* Check both file and directory cluster ranges. */
            if((!(pdos_node->fattribute & ADIRENT) || (pdos_node->fattribute & ADIRENT)) && (ret_val == NU_SUCCESS))
            {
                cl = chk_get_dir_rec_cl(pddrive,pdos_node);
                if(cl)
                {
                    /* Get the cluster value in the FAT table. */
                    ret_val = pc_faxx(pddrive,cl,&cl_fat);
                    if(ret_val == NU_SUCCESS)
                    {
                           if((chk_is_cl_free(pddrive, cl) == NU_TRUE) || (chk_is_cl_free(pddrive, cl_fat) == NU_TRUE))
                           {
                               /* Log Errors. */
                               chk_log_add_error(CHKDSK_LOGID_DDR_CL_VALUE_FREE);

                               /* Delete error. */
                               if(g_chkdsk_mode == CHK_FIX_ERRORS)
                               {
                                    /* Does this directory/file have a LFN record?*/
                                    if(pdir_rec_obj->lfn_start_loc.cl_num)
                                    {
                                        /* Make the LFN records as deleted. */
                                        ret_val = chk_lfn_traverse_records(pdir_rec_obj, chk_lfn_del_records, 0);   
                                    }
									if(ret_val == NU_SUCCESS)
									{
										PC_DRIVE_IO_ENTER(pddrive->dh)
										ret_val = fs_dev_io_proc(pddrive->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, YES);
										PC_DRIVE_IO_EXIT(pddrive->dh)
										if(ret_val == NU_SUCCESS)
										{
											/* Mark the SFN entry as deleted. */
											buff[pdir_rec_obj->pdos_rec_loc->offset] = PCDELETE;

											/* Update the delete marker out to the disk. */
											PC_DRIVE_IO_ENTER(pddrive->dh)
											ret_val = fs_dev_io_proc(pddrive->dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, NO);
											PC_DRIVE_IO_EXIT(pddrive->dh)
										}
									}
                               }
                           }

                        if(ret_val == NU_SUCCESS)
                        {
                            /* Now check the LFN entry if there is one. */
                            if(pdir_rec_obj->lfn_start_loc.cl_num)
                            {
                                sfn_chksum = chk_sum((UINT8 *)pdos_node);                    
                                ret_val = chk_lfn_traverse_records(pdir_rec_obj, chk_lfn_check, sfn_chksum);            
                            }              
            
                            if(!(pc_isdot(&pdos_node->fname[0], &pdos_node->fext[0])) && 
                               !(pc_isdotdot(&pdos_node->fname[0], &pdos_node->fext[0])) &&
                               (ret_val == NU_SUCCESS))
                            {
                                /* Check sfn for invalid characters. */
                                ret_val = chk_check_sfn_chars(pdos_node);
                            }

                             /* Now make sure it is a directory,  if it is we still have other
                               issues to check. */
                            if((pdos_node->fattribute & ADIRENT) && (ret_val == NU_SUCCESS))
                            {
                                ret_val = chk_check_dir(pdir_rec_obj);
                            }
                        }
                    }        
                }
            }
        }

        /* Reset to success because the error has been logged. */
        if(ret_val == NUF_CHK_CL_INVALID)
        {
            ret_val = NU_SUCCESS;
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
*   chk_dir_rec_check_for_iv_cl_values
*
* DESCRIPTION
*
*   Check for invalid clusters in cluster chains. Fixes only occur if
*   g_chkdsk_mode is equal to CHK_FIX_ERRORS.
*   
*
* INPUTS
*
*   dh(in)                      Disk Handle
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the invalid clusters were fixed,
*                               or did not exist. 
*
*************************************************************************/ 
STATUS chk_dir_rec_check_for_iv_cl_values(UINT16 dh)
{
    STATUS ret_val;

    /* Check for cluster chains for the following invalid
       cluster values and fix if g_chkdsk_mode is CHK_FIX_ERRORS, else 
       just report issue. 
       -Reserved   0x?FF0 - 0x?FF6, 0x1     
       -Cluster is out of range of valid clusters on the drive.  
       */
    ret_val = chk_change_iv_cl_in_fat_to_eoc(dh);    
    if(ret_val == NU_SUCCESS)
    {
        /* Now check for BAD or FREE cluster values that are in
          cluster chains. Verify that the BAD clusters are really bad.
          Clusters are verified as bad by attempting to read from them. If the
          read fails, then the cluster is BAD. */
        ret_val = chk_validate_all_cl_chains_on_disk(dh);
    }
	    
    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*   chk_file_size_test
*
* DESCRIPTION
*
*   Driver function for the file size testing. It
*   starts the file size test logger case, runs the 
*   file size test, then writes out the logger error
*   messages and closes the logger test case.
*
* INPUTS
*  
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the file size test completed.
*
*************************************************************************/
STATUS chk_file_size_test(UINT16 dh)
{
    STATUS ret_val;    

    /* Start the logger. */
    ret_val = chk_start_test_case(CHK_CHECK_FILES_SIZES);    
    
    if(ret_val == NU_SUCCESS)
    {
        /* Make sure our global list of log records gets cleared,
           Need to clear it before each test to make room for
           current test possible errors. */
        CHK_CLEAR_LOG_REC();

        PC_DRIVE_ENTER(dh,YES)
        ret_val = chk_traverse_all_dir_paths(dh, chk_file_size_test_op, NU_TRUE, NU_FALSE);
        PC_DRIVE_EXIT(dh)
        if(ret_val == NU_SUCCESS)
        {
            /* First check to see if we had any errors, if we did out total
               error files found. */
            if((chk_log_is_error_num_zero(CHKDSK_LOGID_IFL_FILES_LEN) == NU_FALSE) || (chk_log_is_error_num_zero(CHKDSK_LOGID_IFL_FILES_GREATER) == NU_FALSE))
            {   
                ret_val = chk_log_write_errors_to_file(CHKDSK_LOGID_IFL_FILES_LEN);
                
                /* Write other error messages to the log. */
                if(ret_val == NU_SUCCESS)
                {
                    if(chk_log_get_error_num(CHKDSK_LOGID_IFL_FILES_GREATER) > 0)
                    {
                        ret_val = chk_log_write_errors_to_file(CHKDSK_LOGID_IFL_FILES_GREATER);                        
                    }
                    
                    if(ret_val == NU_SUCCESS)
                    {
                        if(chk_log_get_error_num(CHKDSK_LOGID_IFL_FILES_LESS) > 0)
                        {
                            ret_val = chk_log_write_errors_to_file(CHKDSK_LOGID_IFL_FILES_LESS);      
                        }
                        
                        /* Write our the finish tag for this test case. */
                        if(ret_val == NU_SUCCESS)
                        {
                            ret_val = chk_end_test_case();
                        }
                    }                    
                }
            }           
        }
    }    

    return ret_val;

}

/************************************************************************
* FUNCTION
*
*   chk_file_size_test_op
*
* DESCRIPTION
*
*   If the cluster chain size is less than the length in the directory 
*   record, then directory record will be updated to the 
*   size of the cluster chain. If the cluster chain size is greater 
*   than the directory record's length the cluster chain will be 
*   terminated an an error file will be created in invalid file length folder
*   with the excess data. If the cluster chain and the directory records 
*   size are equal then nothing will be done.
*   
*
* INPUTS
*
*   *pdir_rec_obj(in)           Pointer to a DIR_REC_OBJ struct.                    
*
* OUTPUTS
*
*   NU_SUCCESS                  If the file size issue was 
*                               resolved.
*   NUF_BADPARM                 pdir_rec_obj is NU_NULL
*
*************************************************************************/
STATUS chk_file_size_test_op(DIR_REC_OBJ *pdir_rec_obj)
{
    STATUS ret_val = NU_SUCCESS;    
    UINT8  buff[CHKDSK_SECTOR_SIZE];           
    UINT32 fl_cl_num;    
    UINT32 disk_sz_calc;
    UINT32 err_fl_sz;
    /* Used to traverse the file(this isn't error file) up to its last cluster. */
    UINT32 curr_file_cl;
    /* Used to point at the cluster where the excess data starts in the file. */
    UINT32 excess_fl_cl;  
    CHAR err_fl_name[12] = {'\0'};    
    UINT32 i;              
    UINT16 dh;
    FAT_CB *pfs_cb = NU_NULL;
    DOSINODE *pdos_node;
    DOSINODE *pnew_dir_rec;
    DDRIVE *pddrive;    
	/* Add one for /0 and one for '.' */    
	static CHAR err_dir_name[MAX_SFN + MAX_EXT + 2] = {CHK_FOLDER_FOR_INVALID_FILE_LEN_ENTRIES};    
    
    if(pdir_rec_obj != NU_NULL)
    {
        pdos_node = pdir_rec_obj->pdos_rec;
        dh = pdir_rec_obj->dh;
    
        /* Make sure this is a file. */
        if(!(pdos_node->fattribute & ADIRENT) && (pdos_node->fattribute != CHK_LFN_ATTR))
        {    
            /* Convert disk handle to a drive number */
            ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);

            if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
            {               
                pddrive = pfs_cb->ddrive;
                
                /* Parse the directory record' cluster chain's start value. */
                curr_file_cl = chk_get_dir_rec_cl(pddrive,pdos_node);
                
                ret_val = chk_calc_size_on_disk(&disk_sz_calc, pdir_rec_obj->dh, curr_file_cl);           
                
                if((ret_val == NU_SUCCESS) && curr_file_cl)
                {
                    /* If the file size in the directory record is larger than the cluster
                       chain's calculated value, then just change the directory record's
                       size value to the calculated value. */
                    if(disk_sz_calc < pdos_node->fsize)
                    {
                        if(g_chkdsk_mode == CHK_FIX_ERRORS)
                        {
                            /* Read in the sector that contains this directory record. */
                            PC_DRIVE_IO_ENTER(dh)
                            ret_val = fs_dev_io_proc(dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, YES);
                            PC_DRIVE_IO_EXIT(dh)
              
                            if(ret_val == NU_SUCCESS)
                            {
                                /* Update the file size. */
                                pnew_dir_rec = (DOSINODE*)&buff[pdir_rec_obj->pdos_rec_loc->offset];                
                                pnew_dir_rec->fsize = disk_sz_calc;

                                /* Now write it back out to disk. */
                                PC_DRIVE_IO_ENTER(dh)
                                ret_val = fs_dev_io_proc(dh, pdir_rec_obj->pdos_rec_loc->sec_num, &buff[0], 1, NO);
                                PC_DRIVE_IO_EXIT(dh)

                                /* Notify that traversal function(chk_traverse_all_dir_paths) 
                                   that a disk update occurred. */
                                pdir_rec_obj->disk_updated = NU_TRUE;  
                            }
                        }
                    
                        /* Log the error, even if ret_val is an error. */
                        chk_log_add_error(CHKDSK_LOGID_IFL_FILES_LESS);              
            
                    }
                    /* This means the number of cluster on the disk exceed size in directory record. */
                    else
                    {   
                        fl_cl_num = pdos_node->fsize/pddrive->bytespcluster;
                        /* If there is a remainder then data doesn't end on cluster boundary,
                           so add extra cluster to calculated size of file. */
                        if(pdos_node->fsize % pddrive->bytespcluster)
                        {
                            fl_cl_num += 1;
                        }
                        
                        /* Now see if our calculated disk size exceeds,
                           this. */                         
                        if(disk_sz_calc > (fl_cl_num * pddrive->bytespcluster))
                        {
                            /* Log the error. */
                            chk_log_add_error(CHKDSK_LOGID_IFL_FILES_GREATER);
            
                            if(g_chkdsk_mode == CHK_FIX_ERRORS)
                            {
                                /* Release our drive lock so we can do high level file operations. */
                                PC_DRIVE_EXIT(dh)

                                /* Create an error folder for the invalid file length entries.
                                   Then set the drive to it, so the error entries are 
                                   created in it. */
                                if(chk_log_get_error_num(CHKDSK_LOGID_IFL_FILES_GREATER) == 1)
                                {                                        
                                    /* Make sure we are in the root directory. */
                                    ret_val = NU_Set_Current_Dir("\\");
                                    if(ret_val == NU_SUCCESS)
                                    {
                                        /* Make sure string is null terminated is this is new folder for error files/directories. */
                                        err_dir_name[NUF_Get_Str_Len(CHK_FOLDER_FOR_INVALID_FILE_LEN_ENTRIES)] = '\0';
                                        
                                        /* Create a system folder to store all error file lengths. */                                        
                                        ret_val = chk_make_err_dir(&err_dir_name[0]);
                                        if(ret_val == NU_SUCCESS)
                                        {
                                            ret_val = NU_Set_Attributes(&err_dir_name[0], ADIRENT | ASYSTEM | AHIDDEN);                                    
                                        }                                        
                                    }                                         
                                }
                                 /* Set current directory to our error directory. */
                                if(ret_val == NU_SUCCESS)
                                {
                                    ret_val = NU_Set_Current_Dir(&err_dir_name[0]);                                
                                }                                

                                if(ret_val == NU_SUCCESS)
                                {
                                    /* Reacquire our drive lock. */
                                    PC_DRIVE_ENTER(dh,YES)        
                                    
                                    /* Determine where our file ends, subtract one because
                                       curr_file_cl is already in the files cluster chain. */
                                    for(i = fl_cl_num-1; (((i > 0) && (ret_val == NU_SUCCESS) && curr_file_cl)) ;--i)
                                    {                                                                      
                                        ret_val = pc_clnext(&curr_file_cl,pddrive,curr_file_cl);  

                                    }

                                    ret_val = pc_clnext(&excess_fl_cl,pddrive,curr_file_cl);

                                    if((ret_val == NU_SUCCESS) && excess_fl_cl)
                                    {        
                                        /* Create the error file name. */
                                        ret_val = chk_create_error_name(&err_fl_name[0], CHK_INVALID_FILE_LEN_NAME, chk_log_get_error_num(CHKDSK_LOGID_IFL_FILES_GREATER), NU_TRUE);
                                        if(ret_val == NU_SUCCESS)
                                        {                                    
                                            err_fl_sz = disk_sz_calc - (fl_cl_num * pddrive->bytespcluster);
                                            if(err_fl_sz % pddrive->bytespcluster)
                                                err_fl_sz += pddrive->bytespcluster;

                                            /* Create a new file in the root directory with the excess data. */                                        
                                            ret_val = chk_create_error_record(dh,pddrive,&err_dir_name[0],
                                                                              CHK_INVALID_FILE_LEN_NAME, chk_log_get_error_num(CHKDSK_LOGID_IFL_FILES_GREATER),
                                                                              excess_fl_cl,err_fl_sz,NU_TRUE);

                                            if(ret_val == NU_SUCCESS)
                                            {                                            
                                                /* Mark the original files last cluster as EOC. 
                                                   NOTE: This function will handle FAT12/16/32 so just pass
                                                   in FAT32. */
                                                ret_val = pc_pfaxx(pddrive, curr_file_cl, (CHK_FAT32_CL_MASK & CHK_EOC));                
                                                if(ret_val == NU_SUCCESS)
                                                {
                                                    PC_FAT_ENTER(pddrive->dh)
                                                    ret_val = pc_flushfat(pddrive);
                                                    PC_FAT_EXIT(pddrive->dh)
                                                }               
                                    
                                                if(ret_val == NU_SUCCESS)
                                                {
                                                    /* Notify that traversal function(chk_traverse_all_dir_paths) 
                                                       that a disk update occurred. */
                                                    pdir_rec_obj->disk_updated = NU_TRUE;                                                    

                                                    /* Release our drive lock so we can do high level file operations. */
                                                    PC_DRIVE_EXIT(dh)

                                                    /* Make sure we set the current directory back to root. */
                                                    ret_val = NU_Set_Current_Dir("\\");

                                                    /* Reacquire our drive lock. */
                                                    PC_DRIVE_ENTER(dh,YES)      
            
                                                }                                             
                                            }
                                        }
                                    }
                                }
                            }
                        }    
                    }
                }
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
*   chk_calc_file_size
*
* DESCRIPTION
*
*   This function will calculate the size of a file whose
*   directory record is passed in though the DOSINODE structure.
*   The size is calculated by traversing the cluster chain
*   and then traversing the last cluster's sector list until
*   we encounter a sector with a 0x00.
*   NOTE: There is a possibility of an error here if an invalid
*   sector comes after a valid sector that is full of data
*   that invalid sector will be included in the size calculation.
*   There is no way around this.
*
* INPUTS
*
*   *pcalc_file_size(out)       The calculated size of the file,
*                               by the function.
*   dh(in)                      Disk Handle
*   start_cl(in)                Where to start calculating
*                               the file size.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the file size was calculated
*                               correctly.
*
*************************************************************************/
STATUS chk_calc_file_size(UINT32 *pcalc_file_size, UINT16 dh, UINT32 start_cl)
{
    STATUS ret_val;
    UINT32 curr_file_cl, curr_file_cl_value;
    UINT16 buff_offset;
    UINT32 eoc_marker;    
    UINT8  buff[CHKDSK_SECTOR_SIZE];            
    UINT8  end_of_sec = NU_FALSE;
    UINT32 curr_sec;
    UINT32 sec_offset;
    DDRIVE *pddrive;
    FAT_CB *pfs_cb = NU_NULL;

    /* Convert disk handle to a fat control block. */
    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);    

    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL)) 
    {    
        pddrive = pfs_cb->ddrive;

    	curr_file_cl = start_cl;        
        
        /* Determine the start cluster of this directory record. */
        eoc_marker = chk_get_eoc_value_for_drive(pddrive);
        
        /* Zero out the calculated file size to start with. */
        *pcalc_file_size = 0;

        /* Make sure buff_offset starts at zero. */
        buff_offset = 0;
   
        /* Traverse the cluster chain, calculating the actually size of the file. */
        do
        {   
            /* See if this is the last element in our cluster chain. */
            PC_FAT_ENTER(dh)            
            ret_val = pc_clnext(&curr_file_cl_value,pddrive,curr_file_cl);
            PC_FAT_EXIT(dh)
             
            if(ret_val == NU_SUCCESS)
            {
                /* pc_clnext will set the next cluster value to zero when it reaches the eoc,
                   so set our cluster value to eoc.*/
                if(!curr_file_cl_value)
                    curr_file_cl_value = eoc_marker;

                /* If we are looking at the last cluster, count the number of bytes by traversing
                   the sectors for that cluster. */
                if(curr_file_cl_value >= eoc_marker)
                {                           
                    curr_sec = pc_cl2sector(pddrive, curr_file_cl);
                    sec_offset = 0;
                    while((sec_offset < pddrive->secpalloc) && (end_of_sec != NU_TRUE))
                    {
                        PC_DRIVE_IO_ENTER(dh)
                        ret_val = fs_dev_io_proc(dh, curr_sec, &buff[0], 1, YES);
                        PC_DRIVE_IO_EXIT(dh)
                        if(ret_val == NU_SUCCESS)
                        {
                             /* If the first entry is zero we know we have to go back towards
                               start_sec. */
                            if(buff[0] == 0x00)
                            {                               
                                buff_offset = 0;
                                break;
                            }
                            else
                            {                                    
                                ret_val = chk_find_end_of_data_sector(&buff_offset, &buff[0]);                                    
                                if(ret_val == NU_SUCCESS)
                                {
                                    /* Add one because this is an array index and we want
                                       actually size. */
                                    ++buff_offset;
                                    
                                    /* If the sector isn't full then we have found our last sector
                                       in the list. */
                                    if(buff_offset != CHKDSK_SECTOR_SIZE)
                                    {
                                        end_of_sec = NU_TRUE;
                                    }
                                    else
                                    {
                                        /* Reset our buffer offset because we are currently on the
                                           end of a sector boundary. */
                                        buff_offset = 0;
                                    }
                                }                                        
                            
                                if(end_of_sec != NU_TRUE)
                                {
                                    ++sec_offset;
                                    ++curr_sec;
                                }
                            }                                    

                        }                            
                        
                    }
                    /* We have reached the end of the sector list for the last cluster 
                       so set our flag. */
                    end_of_sec = NU_TRUE;

                    /* Now add the size from traversing the sectors. */
                    *pcalc_file_size += (sec_offset) * pddrive->bytspsector + buff_offset;            
                
                
                }
                /* Keep traversing the cluster chain until we find the last cluster. */
                else
                {
                    /* Add in the bytes from a cluster. */
                    *pcalc_file_size += pddrive->bytespcluster;
    
                    /* Now move to the next cluster. */
                    PC_FAT_ENTER(dh)
                    ret_val = pc_clnext(&curr_file_cl_value,pddrive,curr_file_cl);
                    PC_FAT_EXIT(dh)                    
                       
                    /* pc_clnext will set the next cluster value to zero when it reaches the eoc,
                       so set our cluster value to eoc.*/
                    if(!curr_file_cl_value)
                        curr_file_cl_value = eoc_marker;
                        
                    if((curr_file_cl_value < eoc_marker) && (ret_val == NU_SUCCESS))
                    {
                        curr_file_cl = curr_file_cl_value;
                    }
                }
            }

        }while((end_of_sec != NU_TRUE) && (ret_val == NU_SUCCESS));        
    }    

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_calc_size_on_disk
*
* DESCRIPTION
*
*   This function will calculate the size of a file on disk.
*
* INPUTS
*
*   *pcalc_file_size(out)       The calculated size of the file,
*                               by the function.
*   dh(in)                      Disk Handle
*   start_cl(in)                Where to start calculating
*                               the file size.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the file size was calculated
*                               correctly.
*
*************************************************************************/
STATUS chk_calc_size_on_disk(UINT32 *pcalc_file_size,UINT16 dh,UINT32 start_cl)
{
    STATUS ret_val;
    UINT32 curr_file_cl;   
    UINT32 eoc_marker;            
    DDRIVE *pddrive;
    FAT_CB *pfs_cb = NU_NULL;

    /* Convert disk handle to a fat control block. */
    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);    

    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL)) 
    {    
        pddrive = pfs_cb->ddrive;

    	curr_file_cl = start_cl;      
        
        /* Determine the EOC marker. */
        eoc_marker = chk_get_eoc_value_for_drive(pddrive);

        /* Zero out the calculated file size to start with. */
        *pcalc_file_size = 0;   
        
        /* Traverse the cluster chain, calculating the actually size on disk. */
        do
        {
            /* See if this is the last element in our cluster chain. */
            PC_FAT_ENTER(dh)
            ret_val = pc_faxx(pddrive, curr_file_cl, &curr_file_cl);
            PC_FAT_EXIT(dh)

            if(ret_val == NU_SUCCESS)
            {   
                /* Make sure cluster value is valid. */
                ret_val = chk_is_cl_valid(pddrive,curr_file_cl);
                
                if(ret_val == NU_SUCCESS)
                {
                    /* Add in the bytes from a cluster. */
                    *pcalc_file_size += pddrive->bytespcluster;                       
                }               
            }
            
        }while((curr_file_cl < eoc_marker) && (ret_val == NU_SUCCESS));         
    }    

    return ret_val;
}

/************************************************************************
* FUNCTION
*
*   chk_find_end_of_data_sector
*
* DESCRIPTION
*
*   Does a binary search on a block of sector data searching for
*   where the actually data ends(0x00). This is only used on
*   file's sectors that contain data. This function shouldn't
*   be used on directories.
*   
*
* INPUTS
*
*   *plast_valid_element(out)   The offset into sec_data that contains
*                               the last valid element in sec_data. 
*                               A valid element is any non 0x00 value.
*                               NOTE: last_valid_element is set to 0 
*                               if the first element in sec_data is 0x00.
*   *psec_data(in)              Buffer containing sector data information.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the last valid element in sec_data
*                               was found.
*   NUF_BADPARM                 psec_data is empty or NU_NULL.  
*
*************************************************************************/ 
STATUS chk_find_end_of_data_sector(UINT16 *plast_valid_element, UINT8 *psec_data)
{
    STATUS ret_val = NUF_BADPARM;
    UINT16 start_buff_offset = 0;
    UINT16 end_buff_offset = CHKDSK_SECTOR_SIZE;
    UINT16 mid_buff_offset = 1;    
    
    if((psec_data != NU_NULL) && (psec_data[0] != 0x00))
    {
        if(psec_data[CHKDSK_SECTOR_SIZE-1] == 0x00)
        {           
            /* Now do a binary search on sec_data looking for the offset that is 0x00. */
            while((start_buff_offset <= end_buff_offset) && (ret_val != NU_SUCCESS))
            {              
                /* Calculate the new mid index to try. */
                mid_buff_offset = (start_buff_offset + end_buff_offset) / 2;             
    
                /* The value that we are looking for is where the first 0x00 occurs. */
                if((psec_data[mid_buff_offset] == 0x00) && 
                    (psec_data[((mid_buff_offset + 1) >= CHKDSK_SECTOR_SIZE) ? mid_buff_offset : (mid_buff_offset + 1)] == 0x00) 
                   && (psec_data[mid_buff_offset - 1] != 0x00))
                {
                    ret_val = NU_SUCCESS;
                }
                /* We need to keep looking. */
                else
                {                                          
                    /* If it isn't equal to 0x00 then we know we must try range 
                       from mid to end. */
                    if(psec_data[mid_buff_offset] != 0x00)
                    {                
                        start_buff_offset = mid_buff_offset + 1;                        
                    }
                    else
                    {             
                        end_buff_offset = mid_buff_offset - 1;                        
                    }

                }


            }   
            /* If the end of the sector was found subtract one because we want to return the 
               last valid element as an index into the sec_data buffer. */
            if(ret_val == NU_SUCCESS)
                *plast_valid_element = mid_buff_offset - 1;        

        }
        /* This means the buffer is full so return the last element. */
        else
        {
            ret_val = NU_SUCCESS;
            *plast_valid_element = CHKDSK_SECTOR_SIZE - 1;
        }
    }
    else
        *plast_valid_element = 0;
    
    

    return ret_val;

}

/************************************************************************
* FUNCTION
*
*   chk_change_iv_cl_in_fat_to_eoc
*
* DESCRIPTION
*
*   Check the FAT table for the following invalid clusters:
*   -Reserved   0x?FF0 - 0x?FF6, 0x1     
*   -Cluster is out of range of valid clusters on the drive.  
*   
*
* INPUTS
*
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the invalid clusters were fixed,
*                               or did not exist. 
*
*************************************************************************/ 
STATUS chk_change_iv_cl_in_fat_to_eoc(UINT16 dh)
{
    STATUS ret_val;
    FAT_CB *pfs_cb = NU_NULL; 
    UINT32 fat_size = 2; /* First two(0 and 1) clusters in FAT table are reserved. */
    UINT32 cl_value;    
    UINT32 eoc_marker;    
    DDRIVE *pddrive;

    ret_val = fsdh_get_fs_specific(dh, (VOID**)&pfs_cb);
    if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
    {
        pddrive = pfs_cb->ddrive;

        PC_DRIVE_ENTER(dh,YES)

        /* Loop though fat table changing all IV_CL to EOC markers.*/ 
        while(fat_size <= pddrive->maxfindex)
        {
            ret_val = pc_faxx(pddrive, fat_size, &cl_value);
            if(ret_val == NU_SUCCESS)
            {
                /* See if cluster is valid value. */
                ret_val = chk_is_cl_valid(pddrive, cl_value);
                if(ret_val == NUF_CHK_CL_INVALID)
                {
                    /* Determine if it was a bad or free cluster, if it is 
                       ignore it as next tests will get it. */
                    if((chk_is_cl_bad(pddrive, cl_value) == NU_FALSE) && (chk_is_cl_free(pddrive, cl_value) == NU_FALSE))
                    {
                        /* Log the error. */                        
                        chk_log_add_ddr_iv_cl_err(pddrive, cl_value);
                        
                        /* If the user wants us to fix it, then fix it.*/
                        if(g_chkdsk_mode == CHK_FIX_ERRORS)
                        {
                            /* Get EOC marker specific to FAT type. */
                            eoc_marker = chk_get_eoc_value_for_drive(pddrive);
                        
                            /* Change cluster value to EOC. */
                            ret_val = pc_pfaxx(pddrive, fat_size, eoc_marker);
							if(ret_val != NU_SUCCESS)
							{
								break;
							}
                        }
                    }

                    ret_val = NU_SUCCESS;
                }                
            }
			            
			++fat_size;
        }

        /* Make sure fat gets flushed and updated on disk.*/
        if(ret_val == NU_SUCCESS)
        {
            PC_FAT_ENTER(dh)
            ret_val = pc_flushfat(pddrive);
            PC_FAT_EXIT(dh)
        }

        PC_DRIVE_EXIT(dh)
    }
    
  	return (ret_val);
    
}

/************************************************************************
* FUNCTION
*
*   chk_validate_all_cl_chains_on_disk
*
* DESCRIPTION
*
*   Traverses every directory record on the disk passing each
*   directory record it encounters into chk_validate_cl_chains
*   for processing.
*   
*
* INPUTS
*
*   dh(in)                      Disk Handle.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the invalid clusters were fixed,
*                               or did not exist. 
*
*************************************************************************/ 
STATUS chk_validate_all_cl_chains_on_disk(UINT16 dh)
{
    STATUS ret_val;

    ret_val = chk_traverse_all_dir_paths(dh, chk_validate_cl_chains, NU_TRUE, NU_FALSE);
    
    return (ret_val);
}

/************************************************************************
* FUNCTION
*
*   chk_validate_cl_chains
*
* DESCRIPTION
*
*   This functions determines if a cluster chain contains a BAD or FREE
*   cluster value in its chain, using pdir_rec_obj->pdos_rec as the 
*   start cluster. This function is called from chk_traverse_all_dir_paths
*   for every directory record on the disk. 
*   
*
* INPUTS
*
*   *pdir_rec_obj(in)            Directory Record Object.
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the invalid clusters were fixed,
*                               or did not exist. 
*   NUF_BADPARM                 If a parameter is invalid.
*
*************************************************************************/ 
STATUS chk_validate_cl_chains(DIR_REC_OBJ *pdir_rec_obj)
{
    STATUS ret_val;
    UINT32 cl;
    UINT32 prev_cl;
    UINT32 cl_value;
    UINT32 eoc_marker;
    FAT_CB *pfs_cb = NU_NULL;        
    DDRIVE *pddrive;        
    
    if(pdir_rec_obj != NU_NULL)
    {
        ret_val = fsdh_get_fs_specific(pdir_rec_obj->dh, (VOID**)&pfs_cb);
        if((ret_val == NU_SUCCESS) && (pfs_cb != NU_NULL))
        {
            pddrive = pfs_cb->ddrive;  

            /* Get EOC specific to FAT type. */
            eoc_marker = chk_get_eoc_value_for_drive(pddrive);
            
            prev_cl = chk_get_dir_rec_cl(pddrive, pdir_rec_obj->pdos_rec);
            PC_FAT_ENTER(pdir_rec_obj->dh)
            /* Traverse this cluster chain seeing if we find any 
               BAD or FREE cluster values. */
            do
            {
                ret_val = pc_clnext(&cl, pddrive, prev_cl); 

                /* Make sure cluster value returned, isn't other
                   type of invalid clusters, as these are handled in
                   chk_change_iv_cl_in_fat_to_eoc. */
                if((chk_is_cl_range(pddrive, cl) == NU_TRUE) || (chk_is_cl_res(pddrive, cl) == NU_TRUE))
                {
                    ret_val = NU_SUCCESS;
                    break;
                }                    
                
                /* This means next cluster in chain is defective(BAD Cluster value). */
                if(ret_val == NUF_DEFECTIVEC)
                {
                    ret_val = chk_handle_bad_cl_in_cl_chain(pddrive, pdir_rec_obj, prev_cl, cl);
                    break;
                }
                else if(ret_val == NU_SUCCESS)
                {                    
                    /* See if next cluster value is FREE, if it is 
                       handle error. */   
                    ret_val = pc_faxx(pddrive, cl, &cl_value);
                    if((ret_val == NU_SUCCESS) && (cl_value == 0))
                    {
                        /* Handle case where free cluster value
                           is in the middle of a cluster chain. */
                        ret_val = chk_handle_free_cl_in_cl_chain(pddrive, pdir_rec_obj, prev_cl, cl);
                        break;
                    }
                }
                else
                {
                    /* No change in return value. */

                }
                
                prev_cl = cl;
                
            }while((ret_val == NU_SUCCESS)&&(prev_cl != 0) && (prev_cl < eoc_marker));    
            
            PC_FAT_EXIT(pdir_rec_obj->dh)                
        }
    }    
    else
    {
        ret_val = NUF_BADPARM;

    }
    
    return (ret_val);

}

/************************************************************************
* FUNCTION
*
*   chk_handle_bad_cl_in_cl_chain
*
* DESCRIPTION
*
*   This functions handles the case when a BAD cluster is encountered in a 
*   cluster chain. The function will first verify the cluster is bad, by
*   attempting to read from every sector in the cluster. If all reads pass,
*   then the cluster isn't bad and its value is changed to EOC.
*
* INPUTS
*
*   *pddrive(in)                 Pointer to DDRIVE object.
*   *pdir_rec_obj(in)            Directory Record Object.
*   cl(in)                       Cluster whose value is BAD.
*   cl_value(in)                 BAD cluster value
*   
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the invalid clusters were fixed,
*                               or did not exist. 
*   NUF_BADPARM                 If a parameter is invalid.
*
*************************************************************************/
STATUS chk_handle_bad_cl_in_cl_chain(DDRIVE *pddrive, DIR_REC_OBJ *pdir_rec_obj, UINT32 cl, UINT32 cl_value)
{
    STATUS ret_val = NU_SUCCESS;
    UINT32 start_sec, end_sec, curr_sec;
    UINT16 dh;
    UINT8 buff[CHKDSK_SECTOR_SIZE];
    UINT32 eoc_marker;

    if((pddrive != NU_NULL) && (pdir_rec_obj != NU_NULL))
    {
        dh = pddrive->dh;

        /* Get EOC specific to FAT type. */
        eoc_marker = chk_get_eoc_value_for_drive(pddrive);

        /* Log errors. */
        /* Must pass in */
        chk_log_add_ddr_iv_cl_err(pddrive, cl_value);
        
        /* If the user wants us to fix it, then do it! */
        if(g_chkdsk_mode == CHK_FIX_ERRORS)
        {
            /* Loop though all sectors attempting to read from each one,
               if we can't read them all then cluster value is correct
               this is a BAD cluster. */
            ret_val = pc_faxx(pddrive, cl, &cl_value);
            curr_sec = start_sec = pc_cl2sector(pddrive, cl);                     
            end_sec = start_sec + pddrive->secpalloc;

            PC_DRIVE_IO_ENTER(dh)
            while((curr_sec < end_sec) && ret_val == NU_SUCCESS)
            {
                /* Verify the cluster is actually bad by attempting to read from it. */                     
                ret_val = fs_dev_io_proc(dh, curr_sec, &buff[0], 1, YES);                    
                curr_sec++;                          
            }                    
            PC_DRIVE_IO_EXIT(dh)

            /* If cluster value really isn't bad, then just
            mark it as EOC. */
            if((curr_sec == end_sec) && (ret_val == NU_SUCCESS))
            {
                ret_val = pc_pfaxx(pddrive, cl, eoc_marker);
                /* Make sure fat gets flushed and updated on disk.*/
                if(ret_val == NU_SUCCESS)
                {                
                    ret_val = pc_flushfat(pddrive);         
                }               
            }
            else
            {
                /* If pervious cluster isn't start of cluster
                   chain then mark it is a EOC. If previous
                   cluster is start of cluster chain and bad, do nothing. */
                if(cl != pdir_rec_obj->pdos_rec_loc->cl_num)
                {                    
                    ret_val = pc_pfaxx(pddrive, cl, eoc_marker);  
                    /* Make sure fat gets flushed and updated on disk.*/
                    if(ret_val == NU_SUCCESS)
                    {                
                        ret_val = pc_flushfat(pddrive);   
                    }                   
                }
            }
        }
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return (ret_val);

}

/************************************************************************
* FUNCTION
*
*   chk_handle_free_cl_in_cl_chain
*
* DESCRIPTION
*
*   This functions handles the case when a FREE cluster is encountered in a 
*   cluster chain. 
*
* INPUTS
*
*   *pddrive(in)                 Pointer to DDRIVE object.
*   *pdir_rec_obj(in)            Directory Record Object.
*   prev_cl(in)                  Cluster whose value is cl.
*   cl(in)                       Cluster whose value is FREE.
*   
*                               
*
* OUTPUTS
*
*   NU_SUCCESS                  If the invalid clusters were fixed,
*                               or did not exist. 
*   NUF_BADPARM                 If a parameter is invalid.
*
*************************************************************************/
STATUS chk_handle_free_cl_in_cl_chain(DDRIVE *pddrive, DIR_REC_OBJ *pdir_rec_obj, UINT32 prev_cl, UINT32 cl)
{
    STATUS ret_val;        
    UINT32 eoc_marker;

    if((pddrive != NU_NULL) && (pdir_rec_obj != NU_NULL))
    {
        /* Get EOC specific to FAT type. */
        eoc_marker = chk_get_eoc_value_for_drive(pddrive);

        /* Log function takes cluster value, so make sure 
           we get the cluster value of the cluster 
           that is the issue. */
        ret_val = pc_faxx(pddrive, cl, &cl);
        if(ret_val == NU_SUCCESS)
        {
            /* Log errors. */
            chk_log_add_ddr_iv_cl_err(pddrive, cl);
        
            /* If the user wants us to fix it, then do it! */
            if(g_chkdsk_mode == CHK_FIX_ERRORS)
            {
                /* Mark the previous cluster as EOC. */       
                ret_val = pc_pfaxx(pddrive, prev_cl, eoc_marker);
                if(ret_val == NU_SUCCESS)
                {
                    /* Make sure fat gets flushed and updated on disk.*/
                    ret_val = pc_flushfat(pddrive);                
                }                   
            }
        }        
    }
    else
    {
        ret_val = NUF_BADPARM;
    }

    return (ret_val);
}

#endif /* (CFG_NU_OS_STOR_FILE_VFS_INCLUDE_CHECK_DISK == 1) */

